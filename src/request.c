#include "http.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

static int month_number(const char *name)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int i;
    for (i = 0; i < 12; ++i) if (strncmp(months + i * 3, name, 3) == 0) return i + 1;
    return 0;
}
/* Gregorian civil date to Unix days, independent of TZ and non-POSIX timegm. */
static int64_t civil_days(int year, unsigned month, unsigned day)
{
    int era;
    unsigned yoe, doy, doe;
    year -= month <= 2;
    era = (year >= 0 ? year : year - 399) / 400;
    yoe = (unsigned)(year - era * 400);
    doy = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + doe - 719468;
}
static int http_date(const char *value, time_t *out)
{
    char weekday[16], month[4], zone[4];
    int year, day, hour, minute, second, used = 0, found, m;
    int64_t seconds;
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    found = sscanf(value, "%15[^,], %2d %3s %4d %2d:%2d:%2d %3s%n",
                   weekday, &day, month, &year, &hour, &minute, &second, zone, &used);
    if (found != 8 || value[used]) {
        used = 0;
        found = sscanf(value, "%15[^,], %2d-%3s-%4d %2d:%2d:%2d %3s%n",
                       weekday, &day, month, &year, &hour, &minute, &second, zone, &used);
        if (found == 8 && !value[used]) { if (year < 100) year += year < 70 ? 2000 : 1900; }
        else {
            used = 0;
            found = sscanf(value, "%15s %3s %2d %2d:%2d:%2d %4d%n",
                           weekday, month, &day, &hour, &minute, &second, &year, &used);
            if (found != 7 || value[used]) return -1;
            strcpy(zone, "GMT");
        }
    }
    m = month_number(month);
    if (!m || strcmp(zone, "GMT") || year < 1970 || year > 9999 ||
        day < 1 || day > days[m - 1] + (m == 2 && (year % 4 == 0 &&
        (year % 100 != 0 || year % 400 == 0))) || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) return -1;
    seconds = civil_days(year, (unsigned)m, (unsigned)day) * 86400 + hour * 3600 + minute * 60 + second;
    *out = (time_t)seconds;
    return (int64_t)*out == seconds ? 0 : -1;
}
static int token_char(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || strchr("!#$%&'*+-.^_`|~", c) != NULL;
}
int request_read(int fd, http_request *request, int *error_status)
{
    char buffer[HTTP_MAX_REQUEST + 1], *end = NULL, *line, *first, *target, *version;
    size_t size = 0;
    int seen_date = 0;
    memset(request, 0, sizeof(*request));
    *error_status = 400;
    while (size < HTTP_MAX_REQUEST) {
        ssize_t received = recv(fd, buffer + size, HTTP_MAX_REQUEST - size, 0);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) return -1;
        size += (size_t)received;
        buffer[size] = '\0';
        if (memchr(buffer, '\0', size) != NULL) return -1;
        end = strstr(buffer, "\r\n\r\n");
        if (end) break;
    }
    if (!end) return -1;
    *end = '\0';
    first = strstr(buffer, "\r\n");
    if (first) { *first = '\0'; line = first + 2; }
    else line = end;
    if (strlen(buffer) >= sizeof(request->first_line)) return -1;
    strcpy(request->first_line, buffer);
    target = strchr(buffer, ' ');
    if (!target) return -1;
    *target++ = '\0';
    version = strchr(target, ' ');
    if (!version) return -1;
    *version++ = '\0';
    if (!*buffer || !*target || strchr(version, ' ') ||
        (strcmp(version, "HTTP/1.0") && strcmp(version, "HTTP/1.1"))) return -1;
    for (first = buffer; *first; ++first) if (!token_char((unsigned char)*first)) return -1;
    if (target[0] != '/' && target[0] != '~') return -1;
    for (first = target; *first; ++first)
        if ((unsigned char)*first <= 32 || (unsigned char)*first == 127 || *first == '#') return -1;
    if (strlen(target) >= sizeof(request->target)) return -1;
    strcpy(request->target, target);
    request->method = !strcmp(buffer, "GET") ? METHOD_GET :
                      !strcmp(buffer, "HEAD") ? METHOD_HEAD : METHOD_OTHER;
    while (*line) {
        char *next = strstr(line, "\r\n"), *colon, *value, *tail;
        if (next) *next = '\0';
        colon = strchr(line, ':');
        if (!colon || colon == line) return -1;
        for (first = line; first < colon; ++first)
            if (!token_char((unsigned char)*first)) return -1;
        *colon = '\0';
        value = colon + 1;
        while (*value == ' ' || *value == '\t') ++value;
        tail = value + strlen(value);
        while (tail > value && (tail[-1] == ' ' || tail[-1] == '\t')) *--tail = '\0';
        for (first = value; *first; ++first)
            if (((unsigned char)*first < 32 && *first != '\t') || (unsigned char)*first == 127) return -1;
        if (!strcasecmp(line, "If-Modified-Since")) {
            if (seen_date++) return -1;
            request->has_if_modified_since = http_date(value, &request->if_modified_since) == 0;
        }
        if (!next) break;
        line = next + 2;
    }
    return 0;
}
