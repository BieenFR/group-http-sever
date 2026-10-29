#define _XOPEN_SOURCE 700
#include "http.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
static int failure(int *status)
{
    *status = errno == ENOENT || errno == ENOTDIR ? 404 :
              errno == EACCES || errno == EPERM || errno == ELOOP ? 403 : 500;
    return -1;
}
static int within(const char *root, const char *path)
{
    size_t length = strlen(root);
    return !strncmp(root, path, length) && (length == 1 || path[length] == '/' || path[length] == '\0');
}
/* Open each canonical component relative to root, disallowing symlink races. */
static int open_beneath(const char *root, const char *canonical)
{
    char rest[PATH_MAX], *component, *save;
    int fd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    strcpy(rest, canonical + strlen(root));
    component = strtok_r(rest, "/", &save);
    while (component) {
        char *next = strtok_r(NULL, "/", &save);
        int child = openat(fd, component, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK |
                           (next ? O_DIRECTORY : 0));
        int saved = errno;
        close(fd);
        if (child < 0) { errno = saved; return -1; }
        fd = child;
        component = next;
    }
    return fd;
}
int resource_open(const server_config *config, const char *target,
                  char *resolved, size_t capacity, int *status)
{
    char uri[HTTP_MAX_TARGET], root[PATH_MAX], candidate[PATH_MAX], canonical[PATH_MAX];
    const char *relative;
    struct stat info;
    int fd, length;
    *status = 403;
    if (strlen(target) >= sizeof(uri)) return -1;
    strcpy(uri, target);
    uri[strcspn(uri, "?")] = '\0';
    /* Decode once for directory links; reject malformed escapes and NUL. */
    {
        char *read = uri, *write = uri;
        while (*read) {
            unsigned char c = (unsigned char)*read++;
            if (c == '%') {
                const char *digits = "0123456789abcdef";
                const char *a, *b;
                unsigned char x = (unsigned char)read[0], y;
                if (!x || !read[1]) return -1;
                y = (unsigned char)read[1];
                if (x >= 'A' && x <= 'F') x = (unsigned char)(x + 'a' - 'A');
                if (y >= 'A' && y <= 'F') y = (unsigned char)(y + 'a' - 'A');
                a = strchr(digits, x); b = strchr(digits, y);
                if (!a || !b) return -1;
                c = (unsigned char)((a - digits) * 16 + (b - digits));
                read += 2;
            }
            if (c == 0 || c < 32 || c == 127 || c == '\\') return -1;
            *write++ = (char)c;
        }
        *write = '\0';
    }
    if (uri[0] == '/' && uri[1] == '~') memmove(uri, uri + 1, strlen(uri));
    if (uri[0] == '~') {
        char *slash = strchr(uri, '/');
        struct passwd *user;
        if (slash) *slash = '\0';
        if (!uri[1] || !(user = getpwnam(uri + 1))) { *status = 404; return -1; }
        length = snprintf(candidate, sizeof(candidate), "%s/sws", user->pw_dir);
        if (length < 0 || (size_t)length >= sizeof(candidate)) return -1;
        if (!realpath(candidate, root)) return failure(status);
        relative = slash ? slash + 1 : "";
    } else {
        if (uri[0] != '/') return -1;
        if (!realpath(config->root, root)) return failure(status);
        relative = uri + 1;
    }
    /* Reject parent components even if normalization would land back within root. */
    {
        char copy[HTTP_MAX_TARGET], *part, *save;
        strcpy(copy, relative);
        for (part = strtok_r(copy, "/", &save); part; part = strtok_r(NULL, "/", &save))
            if (!strcmp(part, "..")) return -1;
    }
    length = snprintf(candidate, sizeof(candidate), "%s/%s", root, relative);
    if (length < 0 || (size_t)length >= sizeof(candidate)) return -1;
    if (!realpath(candidate, canonical)) return failure(status);
    if (!within(root, canonical)) return -1;
    fd = open_beneath(root, canonical);
    if (fd < 0) return failure(status);
    if (fstat(fd, &info) < 0) { int saved = errno; close(fd); errno = saved; return failure(status); }
    if (S_ISDIR(info.st_mode)) {
        int index = openat(fd, "index.html", O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
        if (index >= 0) {
            if (fstat(index, &info) < 0 || !S_ISREG(info.st_mode)) {
                close(index); close(fd); *status = 403; return -1;
            }
            close(fd);
            fd = index;
            length = snprintf(candidate, sizeof(candidate), "%s/index.html", canonical);
            if (length < 0 || (size_t)length >= sizeof(candidate)) { close(fd); return -1; }
            strcpy(canonical, candidate);
        } else if (errno != ENOENT) {
            int saved = errno; close(fd); errno = saved; return failure(status);
        }
    }
    if (!S_ISREG(info.st_mode) && !S_ISDIR(info.st_mode)) { close(fd); *status = 403; return -1; }
    if (strlen(canonical) >= capacity) { close(fd); *status = 500; return -1; }
    strcpy(resolved, canonical);
    *status = S_ISDIR(info.st_mode) ? 0 : 200;
    return fd;
}
