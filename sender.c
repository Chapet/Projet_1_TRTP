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

status_code resend(pkt_t * pkt) {
    return STATUS_OK;
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
        fd_set read_fd;
        FD_ZERO(&read_fd);
        FD_SET(socket_fd, &read_fd);
        struct timeval timeout = {0, 500000};
        int isAvailable = select(1, &read_fd, NULL, NULL, &timeout);
        /*
         * Boucler tant que l'ack n'est pas satisfaisant
         * => Boucler sur tout le socket afin de lire tous les acks
         * => Si aucun ack n'est satisfaisant ou pas, renvoyer tt ce que l'on doit renvoyer
         * Attendre si rien/plus reçu et rien/plus rien a renvoyer
         * Faire une fonction qui s'occupe de renvoyer tout ce qui doit etre renvoyer dans sent_packets
         *
         */
        if (isAvailable) {
            char * buf = malloc(11);
            recv(socket_fd, buf, 11, 0);
            pkt_t * pkt = pkt_new();
            pkt_decode(buf, 11, pkt);
        }
        else {
            int * packetsToResend;
            packetsToResend = whichToResend();
            int i;
            for (i=0; i<32; i++) {
                if (packetsToResend[i] != -1) {
                    resend(sent_packets[i].pkt);
                }
            }
        }
    }


    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_payload(&pkt, buf, len);

}




