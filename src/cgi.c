#include "http.h"
#include "response_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef SWS_CGI_TIMEOUT
#define SWS_CGI_TIMEOUT 5
#endif
#define CGI_LIMIT (4U * 1024U * 1024U)
static volatile sig_atomic_t cgi_pid;
static volatile sig_atomic_t interrupted;
static struct sigaction previous_term, previous_int;
static unsigned redirect_depth;

static void cgi_stop(int signo)
{
    int saved = errno;
    void (*old)(int) = signo == SIGTERM ? previous_term.sa_handler : previous_int.sa_handler;
    interrupted = 1;
    if (cgi_pid > 0) kill(-(pid_t)cgi_pid, SIGKILL);
    if (old != SIG_DFL && old != SIG_IGN && old != NULL) old(signo);
    errno = saved;
}
static int64_t milliseconds(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) < 0) return -1;
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static void address(int fd, int remote, char ip[128], char port[16])
{
    struct sockaddr_storage addr;
    socklen_t size = sizeof(addr);
    strcpy(ip, "unknown"); strcpy(port, "0");
    if ((remote ? getpeername(fd, (struct sockaddr *)&addr, &size) :
                  getsockname(fd, (struct sockaddr *)&addr, &size)) == 0)
        (void)getnameinfo((struct sockaddr *)&addr, size, ip, 128, port, 16, NI_NUMERICHOST | NI_NUMERICSERV);
}
static int collect(int client, int script, const server_config *config,
                   const http_request *request, const char *script_name,
                   const char *path_info, const char *query,
                   unsigned char **output, size_t *length)
{
    int channel[2], code = 0, failed = 0, waited = 0, error = 502;
    int64_t deadline;
    pid_t pid;
    struct sigaction action;
    char local[128], remote[128], port[16], remote_port[16];
    *output = NULL; *length = 0;
    address(client, 0, local, port); address(client, 1, remote, remote_port);
    memset(&action, 0, sizeof(action)); action.sa_handler = cgi_stop; sigemptyset(&action.sa_mask);
    interrupted = 0;
    if (pipe(channel) < 0) return 500;
    if (sigaction(SIGTERM, &action, &previous_term) < 0) { close(channel[0]); close(channel[1]); return 500; }
    if (sigaction(SIGINT, &action, &previous_int) < 0) {
        sigaction(SIGTERM, &previous_term, NULL); close(channel[0]); close(channel[1]); return 500;
    }
    pid = fork();
    if (pid < 0) { failed = 1; error = 500; close(channel[0]); close(channel[1]); goto restore; }
    if (pid == 0) {
        char software[] = "SERVER_SOFTWARE=sws/0.2", gateway[] = "GATEWAY_INTERFACE=CGI/1.1";
        char protocol[] = "SERVER_PROTOCOL=HTTP/1.0", path[] = "PATH=/usr/bin:/bin";
        char method[32], name[HTTP_MAX_TARGET + 32], info[HTTP_MAX_TARGET + 32];
        char search[HTTP_MAX_TARGET + 32], host[160], service[64], peer[160];
        char document[8192], uri[HTTP_MAX_TARGET + 32], content[] = "CONTENT_LENGTH=0";
        char *environment[] = {software, gateway, protocol, path, method, name, info,
                               search, host, service, peer, document, uri, content, NULL};
        char *arguments[] = {(char *)script_name, NULL};
        int null;
        close(channel[0]);
        (void)sigaction(SIGTERM, &previous_term, NULL);
        (void)sigaction(SIGINT, &previous_int, NULL);
        signal(SIGPIPE, SIG_DFL); signal(SIGALRM, SIG_DFL);
        if (setpgid(0, 0) < 0) _exit(127);
        null = open("/dev/null", O_RDWR);
        if (null < 0 || dup2(null, STDIN_FILENO) < 0 || dup2(channel[1], STDOUT_FILENO) < 0) _exit(127);
        if (!config->debug) (void)dup2(null, STDERR_FILENO);
        if (null > 2) close(null);
        close(channel[1]);
        if (chdir(config->cgi_dir) < 0 || fcntl(script, F_SETFD, 0) < 0) _exit(127);
        snprintf(method, sizeof(method), "REQUEST_METHOD=%s", request->method == METHOD_HEAD ? "HEAD" : "GET");
        snprintf(name, sizeof(name), "SCRIPT_NAME=%s", script_name);
        snprintf(info, sizeof(info), "PATH_INFO=%s", path_info);
        snprintf(search, sizeof(search), "QUERY_STRING=%s", query);
        snprintf(host, sizeof(host), "SERVER_NAME=%s", local);
        snprintf(service, sizeof(service), "SERVER_PORT=%s", port);
        snprintf(peer, sizeof(peer), "REMOTE_ADDR=%s", remote);
        snprintf(document, sizeof(document), "DOCUMENT_ROOT=%s", config->root);
        snprintf(uri, sizeof(uri), "REQUEST_URI=%s", request->target);
        fexecve(script, arguments, environment);
        _exit(127);
    }
    close(channel[1]);
    (void)setpgid(pid, pid);
    cgi_pid = pid;
    deadline = milliseconds();
    if (deadline < 0 || fcntl(channel[0], F_SETFL, O_NONBLOCK) < 0) { failed = 1; error = 500; }
    deadline += SWS_CGI_TIMEOUT * 1000;
    while (!failed) {
        struct pollfd fds[2] = {{.fd = channel[0], .events = POLLIN}, {.fd = client, .events = 0}};
        int ready;
        unsigned char chunk[8192];
        ssize_t count;
        if (interrupted || milliseconds() >= deadline) { failed = 1; error = 504; break; }
        ready = poll(fds, 2, 100);
        if (ready < 0) { if (errno == EINTR) continue; failed = 1; break; }
        if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) { failed = 1; break; }
        if (!(fds[0].revents & (POLLIN | POLLHUP | POLLERR))) continue;
        count = read(channel[0], chunk, sizeof(chunk));
        if (count < 0) { if (errno == EINTR || errno == EAGAIN) continue; failed = 1; break; }
        if (count == 0) break;
        if ((size_t)count > CGI_LIMIT - *length) { failed = 1; break; }
        {
            unsigned char *next = realloc(*output, *length + (size_t)count + 1);
            if (!next) { failed = 1; error = 500; break; }
            *output = next; memcpy(next + *length, chunk, (size_t)count); *length += (size_t)count;
            next[*length] = '\0';
        }
    }
    close(channel[0]);
    /* A program may close stdout and keep running: wait only within its budget. */
    while (!failed && !waited) {
        pid_t result = waitpid(pid, &code, WNOHANG);
        if (result == pid) { waited = 1; break; }
        if (result < 0 && errno != EINTR) { failed = 1; break; }
        if (interrupted || milliseconds() >= deadline) { failed = 1; error = 504; break; }
        (void)poll(NULL, 0, 20);
    }
    /* Also remove descendants that kept running after the CGI leader exited. */
    kill(-pid, SIGKILL);
    if (!waited) while (waitpid(pid, &code, 0) < 0 && errno == EINTR) {}
    if (!WIFEXITED(code) || WEXITSTATUS(code) != 0) failed = 1;
restore:
    cgi_pid = 0;
    sigaction(SIGTERM, &previous_term, NULL);
    sigaction(SIGINT, &previous_int, NULL);
    if (failed) { free(*output); *output = NULL; *length = 0; return error; }
    return 0;
}
int cgi_respond(int fd, const server_config *config, const http_request *request,
                response_result *result)
{
    server_config cgi_config = *config;
    char uri[HTTP_MAX_TARGET], candidate[HTTP_MAX_TARGET], resolved[4096];
    char script_name[HTTP_MAX_TARGET], path_info[HTTP_MAX_TARGET], *query, *line, *end;
    char type[256] = "", location[768] = "", extra[8192] = "";
    unsigned char *output;
    size_t length, header_length, body_length, cut;
    struct stat info;
    int script = -1, status = 404, error, code = 200, explicit_status = 0, has_length = 0;
    size_t declared_length = 0, response_length;
    *result = (response_result){.status = 500, .content_length = 0};
    if (!config->cgi_dir) return http_send_error(fd, request, 403, result);
    strcpy(uri, request->target);
    query = strchr(uri, '?');
    if (query) *query++ = '\0'; else query = uri + strlen(uri);
    if (strncmp(uri, "/cgi-bin/", 9)) return http_send_error(fd, request, 404, result);
    strcpy(candidate, uri + 8);
    cgi_config.root = config->cgi_dir;
    cut = strlen(candidate);
    while (cut > 0) {
        candidate[cut] = '\0';
        script = resource_open(&cgi_config, candidate, resolved, sizeof(resolved), &status);
        if (script >= 0) break;
        if (status != 404) break;
        {
            char *slash = strrchr(candidate, '/');
            if (!slash || slash == candidate) break;
            cut = (size_t)(slash - candidate);
        }
    }
    if (script < 0) return http_send_error(fd, request, status, result);
    if (fstat(script, &info) < 0 || !S_ISREG(info.st_mode) || !(info.st_mode & 0111) || access(resolved, X_OK) < 0) {
        close(script); return http_send_error(fd, request, 403, result);
    }
    if (snprintf(script_name, sizeof(script_name), "/cgi-bin%s", candidate) >= (int)sizeof(script_name)) {
        close(script); return http_send_error(fd, request, 400, result);
    }
    strcpy(path_info, uri + 8 + cut);
    error = collect(fd, script, config, request, script_name, path_info, query, &output, &length);
    close(script);
    if (error) return http_send_error(fd, request, error, result);
    if (!output || length > CGI_LIMIT) { free(output); return http_send_error(fd, request, 502, result); }
    end = strstr((char *)output, "\r\n\r\n");
    header_length = 4;
    if (!end) { end = strstr((char *)output, "\n\n"); header_length = 2; }
    if (!end || (size_t)(end - (char *)output) > HTTP_MAX_REQUEST ||
        memchr(output, '\0', (size_t)(end - (char *)output))) {
        free(output); return http_send_error(fd, request, 502, result);
    }
    body_length = length - ((size_t)(end - (char *)output) + header_length);
    *end = '\0';
    line = (char *)output;
    while (*line) {
        char *next = strchr(line, '\n'), *colon, *value, *tail;
        if (next) *next = '\0';
        tail = line + strlen(line);
        if (tail > line && tail[-1] == '\r') *--tail = '\0';
        colon = strchr(line, ':');
        if (!colon || colon == line) goto invalid;
        for (tail = line; tail < colon; ++tail) {
            unsigned char c = (unsigned char)*tail;
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || strchr("!#$%&'*+-.^_`|~", c))) goto invalid;
        }
        *colon = '\0'; value = colon + 1;
        while (*value == ' ' || *value == '\t') ++value;
        for (tail = value; *tail; ++tail)
            if ((unsigned char)*tail < 32 || (unsigned char)*tail == 127) goto invalid;
        if (!strcasecmp(line, "Status")) {
            if (explicit_status || strlen(value) < 3 || value[0] < '2' || value[0] > '5' ||
                value[1] < '0' || value[1] > '9' || value[2] < '0' || value[2] > '9' ||
                (value[3] && value[3] != ' ')) goto invalid;
            code = (value[0] - '0') * 100 + (value[1] - '0') * 10 + value[2] - '0';
            explicit_status = 1;
        } else if (!strcasecmp(line, "Content-Type")) {
            if (*type || !*value || strlen(value) >= sizeof(type)) goto invalid;
            strcpy(type, value);
        } else if (!strcasecmp(line, "Location")) {
            if (*location || !*value || strlen(value) >= sizeof(location)) goto invalid;
            strcpy(location, value);
        } else if (!strcasecmp(line, "Content-Length")) {
            char *finish;
            unsigned long long amount;
            if (has_length || !*value || strspn(value, "0123456789") != strlen(value)) goto invalid;
            errno = 0; amount = strtoull(value, &finish, 10);
            if (errno || *finish || amount > SIZE_MAX) goto invalid;
            declared_length = (size_t)amount; has_length = 1;
        } else {
            size_t used = strlen(extra);
            int added;
            if (!strcasecmp(line, "Transfer-Encoding") || !strcasecmp(line, "Connection") ||
                !strcasecmp(line, "Keep-Alive") || !strcasecmp(line, "Upgrade") ||
                !strcasecmp(line, "Trailer") || !strcasecmp(line, "TE") ||
                !strcasecmp(line, "Server") || !strcasecmp(line, "Date")) goto invalid;
            added = snprintf(extra + used, sizeof(extra) - used, "%s: %s\r\n", line, value);
            if (added < 0 || (size_t)added >= sizeof(extra) - used) goto invalid;
        }
        if (!next) break;
        line = next + 1;
    }
    if (*location && !explicit_status) {
        if (location[0] == '/' && body_length == 0) {
            http_request redirected = *request;
            int sent;
            if (redirect_depth >= 5) goto invalid;
            strcpy(redirected.target, location);
            free(output); ++redirect_depth;
            sent = response_send(fd, config, &redirected, 0, result);
            --redirect_depth;
            return sent;
        }
        code = 302;
    }
    if (!*type && !*location && code != 204 && code != 304) goto invalid;
    if (!*type) strcpy(type, "text/plain");
    if (code == 204 || code == 304) body_length = 0;
    response_length = request->method == METHOD_HEAD && has_length && code != 204 && code != 304 ? declared_length : body_length;
    error = http_send_headers_extra(fd, result, code, type, response_length, (time_t)-1, *location ? location : NULL, extra);
    if (error == 0 && request->method != METHOD_HEAD)
        error = http_send_bytes(fd, end + header_length, body_length);
    free(output);
    return error;
invalid:
    free(output);
    return http_send_error(fd, request, 502, result);
}
