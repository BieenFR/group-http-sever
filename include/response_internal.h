#ifndef SWS_RESPONSE_INTERNAL_H
#define SWS_RESPONSE_INTERNAL_H
#include "http.h"
int http_send_bytes(int fd, const void *data, size_t length);
int http_send_headers(int fd, response_result *result, int status,
                      const char *type, size_t length, time_t modified,
                      const char *location);
int http_send_headers_extra(int fd, response_result *result, int status,
                            const char *type, size_t length, time_t modified,
                            const char *location, const char *extra_fields);
int http_send_error(int fd, const http_request *request, int status,
                    response_result *result);
#endif
