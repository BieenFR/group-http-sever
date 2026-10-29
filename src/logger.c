#include "http.h"
#include <errno.h>
#include <string.h>

int log_request(const server_config *config, const char *remote_ip,
                time_t received_at, const char *first_line,
                const response_result *result)
{
    (void)config; (void)remote_ip; (void)received_at;
    (void)first_line; (void)result;
    /* TODO LE AN L01-L02: see docs/TASKS.md. Do not return fake success. */
    errno = ENOSYS;
    return -1;
}
