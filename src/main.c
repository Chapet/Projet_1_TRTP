#include <stdlib.h>
#include <getopt.h>
#include "sender.h"

int main(const int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        printf("Not enough or too much arguments !\n");
        return EXIT_FAILURE;
    }
    char *filename = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        if (opt == 'f') {
            if (argc < 5) {
                printf("Not enough arguments : ipv6 address and/or port number missing !\n");
                return EXIT_FAILURE;
            }
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

    /*
    struct timeval stop, start;
    gettimeofday(&start, NULL);
    */
    status_code ret = scheduler(filename);
    if (ret != STATUS_OK) {
        printf("Error code : %d \n", ret);
        return EXIT_FAILURE;
    }
    /*
    gettimeofday(&stop, NULL);
    
    time_t elapsed_s = stop.tv_sec - start.tv_sec;
    suseconds_t elapsed_us = stop.tv_usec - start.tv_usec;
    
    printf("Time elapsed : %.3f\n", (double) (stop.tv_sec - start.tv_sec) + (double) (stop.tv_usec - start.tv_usec) / 1000);
    */
    close_fds();
    free(filename);
    return EXIT_SUCCESS;
}
