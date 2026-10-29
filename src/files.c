#include "http.h"
#include <errno.h>
#include <string.h>

int resource_open(const server_config *config, const char *target,
                  char *resolved, size_t capacity, int *status)
{
    (void)config; (void)target;
    if (capacity) resolved[0] = '\0';
    *status = 501;
    /* TODO VIET AN F01-F04: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
