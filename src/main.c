#include "http.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    server_config config;
    int result = config_parse(argc, argv, &config);
    if (result == 1) return 0;
    if (result < 0) return 2;
    if (server_run(&config) < 0) {
        perror("sws");
        return 1;
    }
    return 0;
}
