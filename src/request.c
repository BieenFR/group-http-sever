#include "http.h"
#include <errno.h>
#include <string.h>

int request_read(int fd, http_request *request, int *error_status)
{
    (void)fd;
    memset(request, 0, sizeof(*request));
    *error_status = 501;
    /* TODO BAO R01-R04: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
