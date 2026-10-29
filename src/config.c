#define _XOPEN_SOURCE 700
#include "http.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
/* Config points to these buffers until process exit; parse once at startup. */
static char root_path[PATH_MAX], cgi_path[PATH_MAX], log_path[PATH_MAX];

static void usage(FILE *out)
{
    fputs("Usage: sws [-dh] [-c dir] [-i address] [-l file] [-p port] dir\n"
          "  -d  foreground, sequential clients, request logs to stdout\n"
          "  -h  show this help\n"
          "  -c  directory of permitted CGI programs\n"
          "  -i  numeric IPv4/IPv6 listen address (default: all interfaces)\n"
          "  -l  append request logs to file\n"
          "  -p  TCP port, 1..65535 (default: 8080)\n", out);
}

static int directory(const char *input, char *output)
{
    struct stat info;
    if (realpath(input, output) == NULL) return -1;
    if (stat(output, &info) < 0) return -1;
    if (!S_ISDIR(info.st_mode)) { errno = ENOTDIR; return -1; }
    return access(output, R_OK | X_OK);
}

int config_parse(int argc, char **argv, server_config *config)
{
    const char *cgi = NULL, *log = NULL;
    int option;
    *config = (server_config){.port = 8080};
    opterr = 0;
    while ((option = getopt(argc, argv, "dhc:i:l:p:")) != -1) {
        switch (option) {
        case 'd': config->debug = 1; break;
        case 'h': usage(stdout); return 1;
        case 'c': cgi = optarg; break;
        case 'i': config->address = optarg; break;
        case 'l': log = optarg; break;
        case 'p': {
            char *end;
            unsigned long port;
            if (*optarg == '\0' || strspn(optarg, "0123456789") != strlen(optarg))
                goto invalid;
            errno = 0;
            port = strtoul(optarg, &end, 10);
            if (errno || *end || port == 0 || port > 65535) goto invalid;
            config->port = (unsigned short)port;
            break;
        }
        default: goto invalid;
        }
    }
    if (argc - optind != 1) goto invalid;
    if (directory(argv[optind], root_path) < 0) {
        perror("sws: document root"); return -1;
    }
    config->root = root_path;
    if (cgi != NULL) {
        if (directory(cgi, cgi_path) < 0) {
            perror("sws: CGI directory"); return -1;
        }
        config->cgi_dir = cgi_path;
    }
    if (log != NULL) {
        int length;
        char cwd[PATH_MAX];
        if (*log == '\0') goto invalid;
        if (*log == '/') length = snprintf(log_path, sizeof(log_path), "%s", log);
        else {
            if (getcwd(cwd, sizeof(cwd)) == NULL) { perror("sws: getcwd"); return -1; }
            length = snprintf(log_path, sizeof(log_path), "%s/%s", cwd, log);
        }
        if (length < 0 || (size_t)length >= sizeof(log_path)) {
            errno = ENAMETOOLONG; perror("sws: log path"); return -1;
        }
        config->log_file = log_path;
    }
    return 0;
invalid:
    usage(stderr);
    errno = EINVAL;
    return -1;
}
