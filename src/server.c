#include "http.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#ifndef SWS_REQUEST_TIMEOUT
#define SWS_REQUEST_TIMEOUT 15
#endif
#define MAX_LISTENERS 8
#define MAX_WORKERS 128
static volatile sig_atomic_t stopping;
static volatile sig_atomic_t active_client = -1;
static pid_t workers[MAX_WORKERS];
static size_t worker_count;

static void stop_signal(int signo)
{
    int saved = errno;
    (void)signo;
    stopping = 1;
    if (active_client >= 0) (void)shutdown((int)active_client, SHUT_RDWR);
    errno = saved;
}
static void timeout_signal(int signo)
{
    int saved = errno;
    (void)signo;
    if (active_client >= 0) (void)shutdown((int)active_client, SHUT_RDWR);
    errno = saved;
}
static int handler(int signo, void (*function)(int))
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = function;
    sigemptyset(&action.sa_mask);
    return sigaction(signo, &action, NULL);
}
static void diagnostic(const server_config *config, const char *operation)
{
    int saved = errno;
    if (config->debug) fprintf(stderr, "sws: %s: %s\n", operation, strerror(saved));
    else syslog(LOG_ERR, "%s: %s", operation, strerror(saved));
}
static void close_listeners(struct pollfd *listeners, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) close(listeners[i].fd);
}
static int listeners_open(const server_config *config, struct pollfd *listeners,
                          size_t *count)
{
    struct addrinfo hints, *addresses, *address;
    char port[6];
    int result, saved;
    *count = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = config->address ? AI_NUMERICHOST : AI_PASSIVE;
    snprintf(port, sizeof(port), "%u", (unsigned)config->port);
    result = getaddrinfo(config->address, port, &hints, &addresses);
    if (result != 0) {
        fprintf(stderr, "sws: listen address: %s\n", gai_strerror(result));
        errno = EINVAL; return -1;
    }
    for (address = addresses; address != NULL; address = address->ai_next) {
        int fd, yes = 1;
        if (*count == MAX_LISTENERS) { errno = EOVERFLOW; goto fail; }
        fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) {
            if (!config->address && (errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT))
                continue;
            goto fail;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
            goto fail_fd;
        /* Separate IPv4 and IPv6 listeners avoid platform-dependent dual-stack defaults. */
        if (address->ai_family == AF_INET6 &&
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) < 0)
            goto fail_fd;
        if (bind(fd, address->ai_addr, address->ai_addrlen) < 0) {
            if (!config->address && errno == EADDRNOTAVAIL) { close(fd); continue; }
            goto fail_fd;
        }
        if (listen(fd, 64) < 0 || fcntl(fd, F_SETFD, FD_CLOEXEC) < 0 ||
            fcntl(fd, F_SETFL, O_NONBLOCK) < 0) goto fail_fd;
        listeners[*count] = (struct pollfd){.fd = fd, .events = POLLIN};
        ++*count;
        continue;
fail_fd:
        saved = errno; close(fd); errno = saved;
        goto fail;
    }
    freeaddrinfo(addresses);
    if (*count) return 0;
    errno = EADDRNOTAVAIL;
    return -1;
fail:
    saved = errno;
    freeaddrinfo(addresses);
    close_listeners(listeners, *count);
    *count = 0;
    errno = saved;
    return -1;
}

/* Double-fork and acknowledge only after terminal detachment and stdio setup.
 * The caller has opened listeners and checked config before entering here. */
static int detach(void)
{
    int channel[2], fd, saved;
    pid_t pid;
    char ready = 0;
    ssize_t n;
    if (pipe(channel) < 0) return -1;
    pid = fork();
    if (pid < 0) { saved = errno; close(channel[0]); close(channel[1]); errno = saved; return -1; }
    if (pid > 0) {
        close(channel[1]);
        do { n = read(channel[0], &ready, 1); } while (n < 0 && errno == EINTR);
        close(channel[0]);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {}
        return n == 1 && ready == 'R' ? 1 : (errno = EIO, -1);
    }
    close(channel[0]);
    if (setsid() < 0) _exit(1);
    pid = fork();
    if (pid < 0) _exit(1);
    if (pid > 0) _exit(0);
    umask(027);
    if (chdir("/") < 0) _exit(1);
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) _exit(1);
    if (dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 ||
        dup2(fd, STDERR_FILENO) < 0) _exit(1);
    if (fd > STDERR_FILENO) close(fd);
    ready = 'R';
    do { n = write(channel[1], &ready, 1); } while (n < 0 && errno == EINTR);
    close(channel[1]);
    if (n != 1) _exit(1);
    return 0;
}
static void reap_workers(void)
{
    size_t i = 0;
    while (i < worker_count) {
        pid_t result = waitpid(workers[i], NULL, WNOHANG);
        if (result > 0 || (result < 0 && errno == ECHILD))
            workers[i] = workers[--worker_count];
        else ++i;
    }
}
static void workers_stop(void)
{
    size_t i;
    int attempt;
    for (i = 0; i < worker_count; ++i) kill(-workers[i], SIGTERM);
    for (attempt = 0; worker_count && attempt < 20; ++attempt) {
        reap_workers();
        if (worker_count) (void)poll(NULL, 0, 100);
    }
    for (i = 0; i < worker_count; ++i) kill(-workers[i], SIGKILL);
    for (i = 0; i < worker_count; ++i)
        while (waitpid(workers[i], NULL, 0) < 0 && errno == EINTR) {}
    worker_count = 0;
}
static void serve_client(int fd, const server_config *config,
                         const struct sockaddr *peer, socklen_t length)
{
    http_request request;
    response_result result = {.status = 500, .content_length = 0};
    struct timeval timeout = {.tv_sec = SWS_REQUEST_TIMEOUT, .tv_usec = 0};
    char ip[128] = "unknown";
    int error_status = 400, parsed;
    time_t received_at = time(NULL);
    memset(&request, 0, sizeof(request));
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        diagnostic(config, "client socket setup"); return;
    }
    (void)getnameinfo(peer, length, ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
    active_client = fd;
    alarm(SWS_REQUEST_TIMEOUT); /* Total request budget, not a per-recv idle timeout. */
    parsed = request_read(fd, &request, &error_status);
    if (parsed < 0 && (error_status < 400 || error_status > 599)) error_status = 400;
    if (!stopping && response_send(fd, config, parsed == 0 ? &request : NULL,
                                  parsed == 0 ? 0 : error_status, &result) < 0)
        diagnostic(config, "response_send");
    if (!stopping && (config->debug || config->log_file) &&
        log_request(config, ip, received_at, request.first_line, &result) < 0)
        diagnostic(config, "log_request");
    alarm(0);
    active_client = -1;
}

int server_run(const server_config *config)
{
    struct pollfd listeners[MAX_LISTENERS];
    size_t count, i;
    int status = -1, saved, detached;
    stopping = 0;
    worker_count = 0;
    if (handler(SIGPIPE, SIG_IGN) < 0 || handler(SIGTERM, stop_signal) < 0 ||
        handler(SIGINT, stop_signal) < 0 || handler(SIGALRM, timeout_signal) < 0 ||
        handler(SIGCHLD, SIG_DFL) < 0) return -1;
    /* Refuse an unusable log destination before reporting daemon startup success. */
    if (!config->debug && config->log_file) {
        int fd = open(config->log_file, O_WRONLY | O_APPEND | O_CREAT | O_NONBLOCK, 0600);
        struct stat st;
        if (fd < 0) return -1;
        if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
            saved = EINVAL; close(fd); errno = saved; return -1;
        }
        close(fd);
    }
    if (listeners_open(config, listeners, &count) < 0) return -1;
    if (!config->debug) {
        detached = detach();
        if (detached != 0) {
            saved = errno; close_listeners(listeners, count); errno = saved;
            return detached > 0 ? 0 : -1;
        }
        openlog("sws", LOG_PID, LOG_DAEMON);
    }
    while (!stopping) {
        int available;
        reap_workers();
        if (worker_count >= MAX_WORKERS) { (void)poll(NULL, 0, 100); continue; }
        available = poll(listeners, (nfds_t)count, 250);
        if (available < 0) { if (errno == EINTR) continue; goto done; }
        for (i = 0; i < count && !stopping; ++i) {
            struct sockaddr_storage peer;
            socklen_t length = sizeof(peer);
            int fd;
            pid_t child;
            if (listeners[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errno = EIO; goto done;
            }
            if (!(listeners[i].revents & POLLIN)) continue;
            fd = accept(listeners[i].fd, (struct sockaddr *)&peer, &length);
            if (fd < 0) {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ||
                    errno == ECONNABORTED) continue;
                goto done;
            }
            if (config->debug) {
                serve_client(fd, config, (struct sockaddr *)&peer, length);
                close(fd);
                continue;
            }
            child = fork();
            if (child < 0) { saved = errno; close(fd); errno = saved; diagnostic(config, "fork"); continue; }
            if (child == 0) {
                close_listeners(listeners, count);
                if (setpgid(0, 0) < 0) _exit(1);
                serve_client(fd, config, (struct sockaddr *)&peer, length);
                close(fd);
                _exit(0);
            }
            (void)setpgid(child, child); /* Separate group includes future CGI children. */
            workers[worker_count++] = child;
            close(fd);
            if (worker_count == MAX_WORKERS) break;
        }
    }
    status = 0;
done:
    saved = errno;
    close_listeners(listeners, count);
    workers_stop();
    if (!config->debug) {
        if (status < 0) diagnostic(config, "server loop");
        closelog();
        _exit(status == 0 ? 0 : 1);
    }
    errno = saved;
    return status;
}
