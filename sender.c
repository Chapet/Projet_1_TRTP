#include "sender.h"


status_code reader(char * filename) {

    // open filename if fArg, stdin otherwise
    int fd = STDIN_FILENO;
    if (fArg) {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            return E_FILENAME;
        }
    }

    char buf[512];
    status_code status;
    ssize_t nBytes = read(fd, &buf, 512);

    while(nBytes > 0) {
        status = sender(buf);
        if (status != STATUS_OK) {
            close(fd);
            return status;
        }
        nBytes = read(fd, &buf, 512);
    }

    close(fd);
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
    pkt_t packet;

}
