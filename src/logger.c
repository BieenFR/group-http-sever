#include "http.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int log_request(const server_config *config, const char *remote_ip,
                time_t received_at, const char *first_line,
                const response_result *result)
{
    char stamp[32], line[HTTP_MAX_REQUEST * 2 + 512], clean[HTTP_MAX_REQUEST * 2 + 1];
    struct tm tm;
    struct flock lock;
    size_t used = 0, sent = 0;
    int fd, length, status = 0, saved;
    const unsigned char *p = (const unsigned char *)first_line;
    if (!config->debug && !config->log_file) return 0;
    if (!gmtime_r(&received_at, &tm) || !strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%SZ", &tm))
        return -1;
    while (*p && used + 2 < sizeof(clean)) {
        unsigned char c = *p++;
        if (c == '"' || c == '\\') { clean[used++] = '\\'; clean[used++] = (char)c; }
        else clean[used++] = c < 32 || c == 127 ? '?' : (char)c;
    }
    clean[used] = '\0';
    length = snprintf(line, sizeof(line), "%s %s \"%s\" %d %zu\n",
                      remote_ip, stamp, clean, result->status, result->content_length);
    if (length < 0 || (size_t)length >= sizeof(line)) return -1;
    fd = config->debug ? STDOUT_FILENO : open(config->log_file, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK; lock.l_whence = SEEK_SET;
    if (!config->debug) {
        while (fcntl(fd, F_SETLKW, &lock) < 0) {
            if (errno != EINTR) { saved = errno; close(fd); errno = saved; return -1; }
        }
    }
    while (sent < (size_t)length) {
        ssize_t count = write(fd, line + sent, (size_t)length - sent);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { status = -1; break; }
        sent += (size_t)count;
    }
    saved = errno;
    if (!config->debug) {
        lock.l_type = F_UNLCK;
        (void)fcntl(fd, F_SETLK, &lock);
        close(fd);
    }
    errno = saved;
    return status;
}
