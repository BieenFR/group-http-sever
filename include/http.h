#ifndef GROUP_HTTP_H
#define GROUP_HTTP_H
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#define SWS_VERSION "0.1-scaffold"
#define HTTP_MAX_REQUEST 16384
#define HTTP_MAX_TARGET 4096
/* Shared contracts: coordinate changes in this file through a PR. */
typedef struct {
    const char *root, *address, *log_file, *cgi_dir;
    unsigned short port;
    int debug;
} server_config;
typedef enum { METHOD_GET, METHOD_HEAD, METHOD_OTHER } http_method;
typedef struct {
    http_method method;
    char target[HTTP_MAX_TARGET]; /* Raw URI: files.c owns path normalization. */
    char first_line[HTTP_MAX_REQUEST];
    int has_if_modified_since;
    time_t if_modified_since;
} http_request;
typedef struct {
    int status;
    size_t content_length; /* Header value, not number of bytes sent for HEAD. */
} response_result;
/* Bien: return 0 success, 1 help shown, -1 invalid arguments. */
int config_parse(int argc, char **argv, server_config *config);
/* Bien: initialize listener(s), accept clients, coordinate modules; -1 on error. */
int server_run(const server_config *config);
/* Bao: bounded read to CRLF CRLF; 0 success, -1 with HTTP error in error_status.
 * Must not read forever, accept oversized input, or depend on a single recv(). */
int request_read(int fd, http_request *request, int *error_status);
/* Viet An: safe file resolution, MIME via libmagic, directory listing, ~user.
 * Return opened regular-file fd >=0; otherwise -1 and set status.
 * A directory with no index: return -1, status=0; response handles listing.
 * resolved must be an authorized canonical path. Callers close returned fd. */
int resource_open(const server_config *config, const char *target,
                  char *resolved, size_t capacity, int *status);
/* Viet An: status override 0 means normal dispatch, else send error.
 * request may be NULL for a malformed request. Return 0 or -1 on I/O failure.
 * Always initialize result. GET sends body; HEAD does not. */
int response_send(int fd, const server_config *config,
                  const http_request *request, int status_override,
                  response_result *result);
/* Le An: CGI only enabled by -c; execute directly using fork/exec, no shell.
 * Return 0 or -1; initialize result. Viet An dispatches /cgi-bin here. */
int cgi_respond(int fd, const server_config *config,
                const http_request *request, response_result *result);
/* Le An: append one sanitized log line; no logging unless -l or -d. */
int log_request(const server_config *config, const char *remote_ip,
                time_t received_at, const char *first_line,
                const response_result *result);
#endif
