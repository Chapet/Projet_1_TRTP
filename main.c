//
// Created by Hadrien Libioulle on 14/10/2019.
//

#include "main.h"

int main(const int argc, char * argv[]) {
    if (argc < 3 || argc > 4) {
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "f:")) != -1){
        switch (opt) {
            case 'f' :
                fArg = true;
                filename = optarg;
                break;
            case 't' :
                test = true;
        }
    }

    hostname = malloc(sizeof(char)*strlen(argv[optind]));
    char ** endptr = malloc(sizeof(char **));
    port = strtol(argv[optind+1], endptr, 10);

    status_code ret = reader();
    if (ret != STATUS_OK) {
        printf("Error code : %d \n", ret);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
