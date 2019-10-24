#include <stdlib.h>
#include <getopt.h>
#include "sender.h"

int main(const int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        printf("Not enough or too much arguments\n");
        return EXIT_FAILURE;
    }
    char *filename = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        if (opt == 'f') {
            size_t size = sizeof(char) * strlen(optarg) + 1;
            filename = malloc(size);
            memcpy(filename, optarg, size);
        } else {
            printf("Wrong arguments\n");
            return EXIT_FAILURE;
        }
    }

    size_t size = sizeof(char) * strlen(argv[optind]);
    hostname = malloc(size);
    memcpy(hostname, argv[optind], size);

    size = sizeof(char) * strlen(argv[optind + 1]);
    port = malloc(size);
    memcpy(port, argv[optind + 1], size);

    //isSocketReady = false;

    status_code ret = scheduler(filename);
    if (ret != STATUS_OK) {
        printf("Error code : %d \n", ret);
        return EXIT_FAILURE;
    }
    close_fds();
    free(filename);
    return EXIT_SUCCESS;
}
