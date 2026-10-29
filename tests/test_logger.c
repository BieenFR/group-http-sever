#include "http.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char **argv)
{
    server_config config = {0};
    response_result result = {.status = 200, .content_length = 42};
    int i, j, code;
    if (argc != 3) return 2;
    config.log_file = argv[1]; config.debug = atoi(argv[2]);
    if (config.debug == 2) { config.debug = 0; config.log_file = NULL; }
    for (i = 0; i < 5; ++i) {
        pid_t pid = fork();
        if (pid < 0) return 3;
        if (!pid) {
            for (j = 0; j < 20; ++j)
                if (log_request(&config, "127.0.0.1", (time_t)0, "GET /\"quote\\path\r\ninjected HTTP/1.0", &result) < 0) _exit(1);
            _exit(0);
        }
    }
    for (i = 0; i < 5; ++i)
        if (wait(&code) < 0 || !WIFEXITED(code) || WEXITSTATUS(code)) return 1;
    return 0;
}
