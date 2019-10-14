#include "sender.h"

int main(const int argc, char * argv[]) {
    if (argc < 3 || argc > 4) {
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "f:")) != -1){
        if (opt) {
            fArg = true;
            filename = optarg; 
        }
    }
    hostname = malloc(sizeof(char)*strlen(argv[optind]));
    char ** endptr = malloc(sizeof(char **));
    port = strtol(argv[optind+1], endptr, 10);


    return EXIT_SUCCESS;
}

int sender() {

}