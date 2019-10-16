#include "sender.h"


status_code reader(char * filename) {

    // open filename if fArg, stdin otherwise
    int fd = STDIN_FILENO; // stdin (== 0)
    if (filename == NULL) { // arg -f not specified
        fd = open(filename, O_RDONLY); // opening the fd in read-only
        if (fd < 0) {
            return E_FILENAME;
        }
    }
    
    char buf[512]; // buffer of the data that is to become the payload
    // side-note : there is no need to free an variable that wasn't obtained via malloc()/calloc()/realloc()
    status_code status;
    ssize_t nBytes = read(fd, &buf, 512); // the amount read by read() (0 = at or past EOF)

    // Read filename/stdin and send via sender
    while(nBytes > 0) {
        status = sender(buf, nBytes);
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

status_code sender(char * buf, uint16_t len) {
    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_payload(&pkt, buf, len);

    if(!isSocketReady) {
        socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            return E_SOCKET;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6; // IPv6
        hints.ai_socktype = SOCK_DGRAM; // diagram connectionless
        getaddrinfo(hostname, port, &hints, &servinfo); //hostname : addr ipV6 or hostname

        // retrieving values from struct addrinfo
        socklen_t addrlen = servinfo->ai_addrlen;
        struct sockaddr* dest_addr = servinfo->ai_addr;

        int err = connect(socket_fd, dest_addr, addrlen);
        if (err == -1) {
            return E_SOCKET;
        }
        isSocketReady = true;
    }



}
