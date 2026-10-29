#include "http.h"
#include "response_internal.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef SWS_FILE_PROGRAM
#define SWS_FILE_PROGRAM "file"
#endif
#define MAX_LISTING (4U * 1024U * 1024U)
int http_send_bytes(int fd, const void *data, size_t length)
{
    const char *bytes = data;
    while (length) {
        ssize_t count = send(fd, bytes, length, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return -1;
        bytes += count;
        length -= (size_t)count;
    }
    return 0;
}
static const char *reason(int status)
{
    switch (status) {
    case 200: return "OK"; case 201: return "Created"; case 202: return "Accepted";
    case 204: return "No Content"; case 301: return "Moved Permanently";
    case 302: return "Moved Temporarily"; case 304: return "Not Modified";
    case 400: return "Bad Request"; case 401: return "Unauthorized";
    case 403: return "Forbidden"; case 404: return "Not Found";
    case 500: return "Internal Server Error"; case 501: return "Not Implemented";
    case 502: return "Bad Gateway"; case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    default: return "CGI Response";
    }
}
/* Manual English date avoids locale-dependent weekday/month names. */
static int date_string(time_t value, char output[40])
{
    static const char *week[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    struct tm tm;
    if (!gmtime_r(&value, &tm)) return -1;
    snprintf(output, 40, "%s, %02d %s %04d %02d:%02d:%02d GMT", week[tm.tm_wday],
             tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return 0;
}
int http_send_headers_extra(int fd, response_result *result, int status,
                            const char *type, size_t length, time_t modified,
                            const char *location, const char *extra_fields)
{
    char headers[12288], now[40], last[40], extra[1024] = "";
    int count;
    result->status = status;
    result->content_length = length;
    if (date_string(time(NULL), now) < 0) return -1;
    if (modified >= 0) {
        if (date_string(modified, last) < 0) return -1;
        snprintf(extra, sizeof(extra), "Last-Modified: %s\r\n", last);
    }
    if (location) {
        size_t used = strlen(extra);
        if (strchr(location, '\r') || strchr(location, '\n')) return -1;
        count = snprintf(extra + used, sizeof(extra) - used, "Location: %s\r\n", location);
        if (count < 0 || (size_t)count >= sizeof(extra) - used) return -1;
    }
    count = snprintf(headers, sizeof(headers),
        "HTTP/1.0 %d %s\r\nDate: %s\r\nServer: sws/0.2\r\nConnection: close\r\n"
        "%s%sContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
        status, reason(status), now, extra, extra_fields ? extra_fields : "", type, length);
    if (count < 0 || (size_t)count >= sizeof(headers)) return -1;
    return http_send_bytes(fd, headers, (size_t)count);
}
int http_send_headers(int fd, response_result *result, int status,
                      const char *type, size_t length, time_t modified,
                      const char *location)
{
    return http_send_headers_extra(fd, result, status, type, length, modified, location, NULL);
}
int http_send_error(int fd, const http_request *request, int status, response_result *result)
{
    char body[256];
    int length = snprintf(body, sizeof(body), "<!doctype html><html><title>%d %s</title>"
                           "<h1>%d %s</h1></html>\n", status, reason(status), status, reason(status));
    if (length < 0 || (size_t)length >= sizeof(body)) return -1;
    if (http_send_headers(fd, result, status, "text/html; charset=utf-8", (size_t)length, (time_t)-1, NULL) < 0)
        return -1;
    return request && request->method == METHOD_HEAD ? 0 : http_send_bytes(fd, body, (size_t)length);
}
/* file(1) consults magic patterns; use stdin from the verified file fd, no shell
 * and no extension-only guesses. No development libmagic dependency required. */
static int mime_type(int file_fd, char type[128])
{
    int channel[2], code = 0, okay = 0;
    pid_t pid;
    ssize_t length = -1;
    struct pollfd poller;
    if (pipe(channel) < 0) return -1;
    pid = fork();
    if (pid < 0) { close(channel[0]); close(channel[1]); return -1; }
    if (!pid) {
        int null = open("/dev/null", O_WRONLY);
        close(channel[0]);
        if (dup2(file_fd, STDIN_FILENO) < 0 || dup2(channel[1], STDOUT_FILENO) < 0) _exit(127);
        if (null >= 0) { (void)dup2(null, STDERR_FILENO); if (null > 2) close(null); }
        close(channel[1]);
        execlp(SWS_FILE_PROGRAM, "file", "-b", "--mime-type", "-", (char *)NULL);
        _exit(127);
    }
    close(channel[1]);
    poller = (struct pollfd){.fd = channel[0], .events = POLLIN};
    if (poll(&poller, 1, 3000) > 0) length = read(channel[0], type, 127);
    close(channel[0]);
    if (length <= 0) kill(pid, SIGKILL);
    while (waitpid(pid, &code, 0) < 0) { if (errno != EINTR) return -1; }
    if (length > 0 && WIFEXITED(code) && WEXITSTATUS(code) == 0) {
        size_t i;
        type[length] = '\0';
        type[strcspn(type, "\r\n")] = '\0';
        okay = strchr(type, '/') != NULL;
        for (i = 0; type[i]; ++i)
            if (!((type[i] >= 'a' && type[i] <= 'z') || (type[i] >= 'A' && type[i] <= 'Z') ||
                  (type[i] >= '0' && type[i] <= '9') || strchr("/!#$&^_.+-", type[i]))) okay = 0;
    }
    if (lseek(file_fd, 0, SEEK_SET) < 0) return -1;
    return okay ? 0 : -1;
}
static int append(char **buffer, size_t *length, const char *text)
{
    size_t count = strlen(text);
    char *next;
    if (count > MAX_LISTING - *length) return -1;
    next = realloc(*buffer, *length + count + 1);
    if (!next) return -1;
    memcpy(next + *length, text, count + 1);
    *length += count;
    *buffer = next;
    return 0;
}
static char *escaped(const char *text, int url)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t length = strlen(text), i, used = 0;
    char *out = malloc(length * 6 + 1);
    if (!out) return NULL;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        const char *replacement = NULL;
        if (url) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || strchr("-._~", c)) out[used++] = (char)c;
            else { out[used++] = '%'; out[used++] = hex[c >> 4]; out[used++] = hex[c & 15]; }
            continue;
        }
        if (c == '&') replacement = "&amp;";
        if (c == '<') replacement = "&lt;";
        if (c == '>') replacement = "&gt;";
        if (c == '"') replacement = "&quot;";
        if (c == '\'') replacement = "&#39;";
        if (replacement) { size_t n = strlen(replacement); memcpy(out + used, replacement, n); used += n; }
        else out[used++] = (char)c;
    }
    out[used] = '\0';
    return out;
}
static int name_compare(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}
static int directory_body(int fd, const http_request *request, char **body, size_t *length)
{
    DIR *directory = fdopendir(fd);
    struct dirent *entry;
    char **names = NULL;
    size_t count = 0, i;
    int result = -1;
    *body = NULL; *length = 0;
    if (!directory) { close(fd); return -1; }
    errno = 0;
    while ((entry = readdir(directory))) {
        char **next;
        if (entry->d_name[0] == '.') continue;
        if (count >= 10000) goto done;
        next = realloc(names, (count + 1) * sizeof(*next));
        if (!next) goto done;
        names = next;
        names[count] = strdup(entry->d_name);
        if (!names[count]) goto done;
        ++count;
        errno = 0;
    }
    if (errno) goto done;
    qsort(names, count, sizeof(*names), name_compare);
    if (append(body, length, "<!doctype html><html><meta charset=\"utf-8\"><title>Directory index</title><h1>Directory index</h1><ul>\n") < 0) goto done;
    for (i = 0; i < count; ++i) {
        char *html = escaped(names[i], 0), *url = escaped(names[i], 1), *base = NULL;
        struct stat info;
        int is_dir = fstatat(dirfd(directory), names[i], &info, 0) == 0 && S_ISDIR(info.st_mode);
        const char *query = strchr(request->target, '?');
        size_t base_length = query ? (size_t)(query - request->target) : strlen(request->target);
        char path[HTTP_MAX_TARGET];
        if (base_length >= sizeof(path)) { free(html); free(url); goto done; }
        memcpy(path, request->target, base_length); path[base_length] = '\0';
        base = escaped(path, 0);
        if (!html || !url || !base || append(body, length, "<li><a href=\"") < 0 ||
            append(body, length, base) < 0 ||
            (base_length && path[base_length - 1] != '/' && append(body, length, "/") < 0) ||
            append(body, length, url) < 0 || (is_dir && append(body, length, "/") < 0) ||
            append(body, length, "\">") < 0 || append(body, length, html) < 0 ||
            (is_dir && append(body, length, "/") < 0) || append(body, length, "</a></li>\n") < 0) {
            free(html); free(url); free(base); goto done;
        }
        free(html); free(url); free(base);
    }
    if (append(body, length, "</ul></html>\n") < 0) goto done;
    result = 0;
done:
    for (i = 0; i < count; ++i) free(names[i]);
    free(names);
    closedir(directory);
    if (result < 0) { free(*body); *body = NULL; }
    return result;
}
int response_send(int fd, const server_config *config, const http_request *request,
                  int status_override, response_result *result)
{
    char resolved[4096], type[128], data[16384];
    struct stat info;
    int file_fd, status;
    *result = (response_result){.status = 500, .content_length = 0};
    if (status_override) return http_send_error(fd, request, status_override, result);
    if (!request) return http_send_error(fd, NULL, 400, result);
    if (request->method == METHOD_OTHER) return http_send_error(fd, request, 501, result);
    if (!strncmp(request->target, "/cgi-bin/", 9) || !strcmp(request->target, "/cgi-bin")) {
        if (!config->cgi_dir) return http_send_error(fd, request, 403, result);
        return cgi_respond(fd, config, request, result);
    }
    file_fd = resource_open(config, request->target, resolved, sizeof(resolved), &status);
    if (file_fd < 0) return http_send_error(fd, request, status ? status : 500, result);
    if (config->cgi_dir) {
        size_t n = strlen(config->cgi_dir);
        if (!strncmp(resolved, config->cgi_dir, n) &&
            (resolved[n] == '/' || resolved[n] == '\0' || (n == 1 && config->cgi_dir[0] == '/'))) {
            close(file_fd); return http_send_error(fd, request, 403, result);
        }
    }
    if (fstat(file_fd, &info) < 0 || info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        close(file_fd); return http_send_error(fd, request, 500, result);
    }
    if (request->has_if_modified_since && request->if_modified_since <= time(NULL) &&
        info.st_mtime <= request->if_modified_since) {
        close(file_fd);
        return http_send_headers(fd, result, 304, "application/octet-stream", 0, info.st_mtime, NULL);
    }
    if (S_ISDIR(info.st_mode)) {
        char *body;
        size_t length;
        int sent;
        if (directory_body(file_fd, request, &body, &length) < 0)
            return http_send_error(fd, request, 500, result);
        sent = http_send_headers(fd, result, 200, "text/html; charset=utf-8", length, info.st_mtime, NULL);
        if (sent == 0 && request->method != METHOD_HEAD) sent = http_send_bytes(fd, body, length);
        free(body);
        return sent;
    }
    if (mime_type(file_fd, type) < 0) { close(file_fd); return http_send_error(fd, request, 500, result); }
    if (!strcmp(type, "text/plain")) {
        const char *extension = strrchr(resolved, '.');
        if (extension && !strcmp(extension, ".css")) strcpy(type, "text/css");
        else if (extension && !strcmp(extension, ".js")) strcpy(type, "application/javascript");
    }
    if (http_send_headers(fd, result, 200, type, (size_t)info.st_size, info.st_mtime, NULL) < 0) {
        close(file_fd); return -1;
    }
    if (request->method != METHOD_HEAD) {
        size_t left = (size_t)info.st_size;
        while (left) {
            ssize_t count = read(file_fd, data, left < sizeof(data) ? left : sizeof(data));
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0 || http_send_bytes(fd, data, (size_t)count) < 0) { close(file_fd); return -1; }
            left -= (size_t)count;
        }
    }
    close(file_fd);
    return 0;
}
