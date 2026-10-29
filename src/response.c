#include "http.h"
#include <errno.h>
#include <string.h>

int response_send(int fd, const server_config *config,
                  const http_request *request, int status_override,
                  response_result *result)
{
    (void)fd; (void)config; (void)request; (void)status_override;
    *result = (response_result){.status = 501, .content_length = 0};
    /* TODO VIET AN H01-H04: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
