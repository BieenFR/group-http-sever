/* Test doubles ONLY: not an HTTP parser/response implementation.
 * Linked exclusively by test_bien.py, never by the production Makefile. */
#include "http.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int request_read(int fd, http_request *request, int *error_status)
{
    char c;
    ssize_t n;
    memset(request, 0, sizeof(*request));
    do { n = recv(fd, &c, 1, 0); } while (n < 0 && errno == EINTR);
    if (n != 1) { *error_status = 400; return -1; }
    strcpy(request->first_line, "TEST request");
    request->method = METHOD_GET;
    return 0;
}
int response_send(int fd, const server_config *config, const http_request *request,
                  int error_status, response_result *result)
{
    char body[8192];
    int length, open_fds = 0, i, null_stdout;
    char cwd[4096];
    struct stat out, null;
    if (!getcwd(cwd, sizeof(cwd))) return -1;
    for (i = 0; i < 256; ++i) if (fcntl(i, F_GETFD) >= 0) ++open_fds;
    null_stdout = fstat(STDOUT_FILENO, &out) == 0 && stat("/dev/null", &null) == 0 && S_ISCHR(out.st_mode) && out.st_rdev == null.st_rdev;
    size_t sent = 0;
    (void)request;
    result->status = error_status ? error_status : 200;
    length = snprintf(body, sizeof(body), "%ld|%ld|%s|%s|%s|%d|%s|%ld|%d|%d\n",
                      (long)getpid(), (long)getppid(), config->root,
                      config->cgi_dir ? config->cgi_dir : "-",
                      config->log_file ? config->log_file : "-", result->status, cwd, (long)getsid(0), null_stdout, open_fds);
    if (length < 0 || (size_t)length >= sizeof(body)) return -1;
    result->content_length = (size_t)length;
    while (sent < (size_t)length) {
        ssize_t n = send(fd, body + sent, (size_t)length - sent, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}
int log_request(const server_config *config, const char *ip, time_t timestamp,
                const char *line, const response_result *result)
{
    FILE *out = config->debug ? stdout : fopen(config->log_file, "a");
    (void)timestamp;
    if (!out) return -1;
    fprintf(out, "%s %s %d\n", ip, line, result->status);
    if (config->debug) return fflush(out);
    return fclose(out);
}
