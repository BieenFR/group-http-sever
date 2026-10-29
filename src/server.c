#include "http.h"
#include <errno.h>
#include <string.h>

int server_run(const server_config *config)
{
    (void)config;
    /* TODO BIEN B02-B04: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
