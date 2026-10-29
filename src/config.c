#include "http.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
int config_parse(int argc, char **argv, server_config *config)
{
    *config = (server_config){.port = 8080};
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        puts("Usage: sws [-dh] [-c dir] [-i address] [-l file] [-p port] dir");
        puts("Scaffold only: request handling is not implemented yet.");
        return 1;
    }
    /* TODO BIEN B01: getopt, defaults, validation, required document root. */
    fputs("TODO B01: implement config_parse (only -h works).\n", stderr);
    errno = ENOSYS;
    return -1;
}
