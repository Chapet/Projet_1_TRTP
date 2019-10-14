#include "sender.h"

int main(const int argc, char * argv[]) {
    if (argc < 3 || argc > 4) {
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "f:")) != -1){ // argument handling
        switch (opt) {
            case 'f' : // a file has been given to read from
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

status_code reader() {

    // open filename if fArg, stdin otherwise
    int fd = STDIN_FILENO;
    if (fArg) {
        fd = open(filename, O_RDONLY); // opening the fd
        if (fd < 0) {
            return E_FILENAME;
        }
    }
    
    char buf[512]; // buffer of the data that is to become the payload
    // side-note : there is no need to free an variable that wasn't obtained via malloc()/calloc()/realloc()
    status_code status;
    ssize_t nBytes = read(fd, &buf, 512); // the amount read by read() (0 = at or past EOF)

    while(nBytes > 0) {
        status = sender(buf); // block reader() until the packet has been created and send
        if (status != STATUS_OK) {
            close(fd);
            return status;
        }
        nBytes = read(fd, &buf, 512); // reads again an a new value of nBytes is set
    }

    close(fd);

    //checking for errors linked to the reading of the fd
    if (nBytes == 0) {
        return STATUS_OK;
    }
    else if (nBytes < 0) {
        return E_READING;
    }
    else {
        return E_GENERIC;
    }
}

status_code sender(char * buf) {

}
