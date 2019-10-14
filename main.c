#include "main.h"

int main(const int argc, char * argv[]) {
    if (argc < 3 || argc > 4) {
        return EXIT_FAILURE;
    }
    char * filename = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1){
        if (opt=='f') {
            size_t size = sizeof(char) * strlen(optarg) + 1;
            filename = malloc(size);
            memcpy(filename, optarg, size);
        }
        else {
            printf("Wrong arguments\n");
            return EXIT_FAILURE;
        }
    }

    hostname = malloc(sizeof(char)*strlen(argv[optind]));
    char ** endptr = malloc(sizeof(char **));
    port = strtol(argv[optind+1], endptr, 10);

    status_code ret = reader(filename);
    if (ret != STATUS_OK) {
        printf("Error code : %d \n", ret);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
