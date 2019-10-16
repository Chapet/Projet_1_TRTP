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

    if(!isSocketReady) {
        socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            return E_CONNECT;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6; // IPv6
        hints.ai_socktype = SOCK_DGRAM; // diagram connectionless
        if (getaddrinfo(hostname, port, &hints, &servinfo) != 0) { //hostname : addr ipV6 or hostname
            return E_CONNECT;
        }

        // retrieving values from struct addrinfo
        socklen_t addrlen = servinfo->ai_addrlen;
        struct sockaddr* dest_addr = servinfo->ai_addr;

        if (connect(socket_fd, dest_addr, addrlen) == -1) {
            return E_CONNECT;
        }
        isSocketReady = true;
    }

    if (curr_seqnum > window_end) {
        int packetsToResend[32];
        packetsToResend = whichToResend();

        fd_set read_fd;
        FD_ZERO(&read_fd);
        FD_SET(socket_fd, &read_fd);
        struct timeval timeout = {1, 0};
        int isAvailable = select(1, &read_fd, NULL, NULL, &timeout);


    }


    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_payload(&pkt, buf, len);

}

int * whichToResend() {
    int * packetsToResend = malloc(32 * sizeof(int));
    int i;
    for (i=0; i<32; i++) {
        if (time(NULL) - sent_packets[i].timer > retransmission_timer) {
            packetsToResend[i] = i;
        }
        else {
            packetsToResend[i] = -1;
        }
    }
    return packetsToResend;
}
