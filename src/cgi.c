#include "http.h"
#include <errno.h>
#include <string.h>

int cgi_respond(int fd, const server_config *config,
                const http_request *request, response_result *result)
{
    (void)fd; (void)config; (void)request;
    *result = (response_result){.status = 501, .content_length = 0};
    /* TODO LE AN C01-C03: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
