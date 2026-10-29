#include "http.h"
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static http_request parse(const char *wire, int expected)
{
    int pair[2], error;
    http_request request;
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    assert(write(pair[0], wire, strlen(wire)) == (ssize_t)strlen(wire));
    shutdown(pair[0], SHUT_WR);
    assert((request_read(pair[1], &request, &error) == 0) == expected);
    close(pair[0]); close(pair[1]);
    return request;
}
int main(void)
{
    http_request request = parse("GET /a?q=1 HTTP/1.1\r\nHost: test\r\n\r\n", 1);
    assert(request.method == METHOD_GET && !strcmp(request.target, "/a?q=1"));
    request = parse("HEAD / HTTP/1.0\r\n\r\n", 1); assert(request.method == METHOD_HEAD);
    request = parse("DELETE / HTTP/1.0\r\n\r\n", 1); assert(request.method == METHOD_OTHER);
    request = parse("GET / HTTP/1.0\r\nIf-Modified-Since: Sun, 06 Nov 1994 08:49:37 GMT\r\n\r\n", 1);
    assert(request.has_if_modified_since && request.if_modified_since == (time_t)784111777);
    request = parse("GET / HTTP/1.0\r\nIf-Modified-Since: Sunday, 06-Nov-94 08:49:37 GMT\r\n\r\n", 1);
    assert(request.has_if_modified_since && request.if_modified_since == (time_t)784111777);
    request = parse("GET / HTTP/1.0\r\nIf-Modified-Since: Sun Nov  6 08:49:37 1994\r\n\r\n", 1);
    assert(request.has_if_modified_since && request.if_modified_since == (time_t)784111777);
    request = parse("GET / HTTP/1.0\r\nIf-Modified-Since: invalid\r\n\r\n", 1);
    assert(!request.has_if_modified_since);
    parse("GET / HTTP/1.0\r\nIf-Modified-Since: x\r\nIf-Modified-Since: y\r\n\r\n", 0);
    parse("GET http://host/ HTTP/1.0\r\n\r\n", 0);
    parse("GET / HTTP/2.0\r\n\r\n", 0);
    parse("GET / HTTP/1.0\r\nBad name: x\r\n\r\n", 0);
    parse("GET / HTTP/1.0\r\nX: y\nInject: z\r\n\r\n", 0);
    parse("GET / HTTP/1.0\r\n", 0);
    parse("GET /#fragment HTTP/1.0\r\n\r\n", 0);
    parse("GET / HTTP/1.0 trailing\r\n\r\n", 0);
    puts("PASS: 15 parser cases on socketpair, real request.c");
    return 0;
}
