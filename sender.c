#include "sender.h"


status_code reader(char * filename) {

    // open filename if fArg, stdin otherwise
    int fd = STDIN_FILENO; // stdin (== 0)
    if (filename != NULL) { // arg -f not specified
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

/*
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
*/

bool isToResend(uint8_t seqnum) {
    int i;
    for (i=0; i < 31 && sent_packets[i] != NULL; i++) {
        if ((sent_packets[i]->pkt)->Seqnum == seqnum) {
            return true;
        }
    }
    return false;
}

status_code send_pkt(pkt_t * pkt) {
    size_t size = predict_header_length(pkt) + pkt_get_length(pkt) + 2* sizeof(uint32_t);
    char * buf = malloc(size);
    pkt_status_code status = pkt_encode(pkt, buf, &size);
    if (status != PKT_OK) {
        return E_SEND_PKT;
    }
    ssize_t sent = send(socket_fd, buf, size, 0);
    if (sent == -1) {
        return E_SEND_PKT;
    }

    curr_seqnum++;
    buffer_t * sent_pkt = malloc(sizeof(buffer_t));
    sent_pkt->timer = time(NULL);
    sent_pkt->seqnum = pkt->Seqnum;
    sent_pkt->pkt = pkt;

    int i;
    for(i=0; sent_packets[i] != NULL; i++);
    sent_packets[i] = sent_pkt;

    //Encode pkt and send it using send(2)
    return STATUS_OK;
}

void removeFromSent(uint8_t seqnum) {
    int i;
    for(i=0; i<31 && sent_packets[i] != NULL; i++) {
        if(sent_packets[i]->seqnum == seqnum) {
            free(sent_packets[i]->pkt->payload);
            free(sent_packets[i]->pkt);
            free(sent_packets[i]);
            sent_packets[i] = NULL;
        }
    }
    return;
}

void emptySocket() {
    fd_set read_fd;
    FD_ZERO(&read_fd);
    FD_SET(socket_fd, &read_fd);

    int isAvailable = select(1, &read_fd, NULL, NULL, &select_timeout);

    char * buf = malloc(11);

    while(isAvailable) {
        recv(socket_fd, buf, 11, 0);
        pkt_t * pkt = pkt_new();
        pkt_status_code status = pkt_decode(buf, 11, pkt);

        if (status == PKT_OK) {
            if (pkt->Type == 2 && isToResend(pkt->Seqnum)) {
                removeFromSent(pkt->Seqnum);
            }
            else if (pkt->Type == 3 && isToResend(pkt->Seqnum)) {
                send_pkt(pkt);
            }
        }
        isAvailable = select(1, &read_fd, NULL, NULL, &select_timeout);
    }

    free(buf);
}
void resendExpiredPkt() {
    int i;
    for(i=0; sent_packets[i] != NULL; i++) {
        if((time(NULL) - sent_packets[i]->timer) > retransmission_timer) {
            send_pkt(sent_packets[i]->pkt);
        }
    }
}


status_code sender(char * data, uint16_t len) {

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
        int i;
        for(i=0; i<32; i++) {
            sent_packets[i] = NULL;
        }

        curr_seqnum=0;
        window_end=30;
        retransmission_timer = 5;
        deadlock_timeout = 120; // 2min timeout if nothing is received and we can't send anything
        select_timeout = (struct timeval) {.tv_sec = 1, .tv_usec = 0};
        isSocketReady = true;
    }

    emptySocket();
    resendExpiredPkt();
    time_t start = time(NULL);
    while (time(NULL) - start < deadlock_timeout && curr_seqnum > window_end) {
        emptySocket();
        resendExpiredPkt();
    }
    if (curr_seqnum > window_end) {
        return E_TIMEOUT;
    }

    /*
     * Boucler tant que l'ack n'est pas satisfaisant
     * => Boucler sur tout le socket afin de lire tous les acks
     * => Si aucun ack n'est satisfaisant ou pas, renvoyer tt ce que l'on doit renvoyer
     * Attendre si rien/plus reçu et rien/plus rien a renvoyer
     * Faire une fonction qui s'occupe de renvoyer tout ce qui doit etre renvoyer dans sent_packets
     *
     */

    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, window_end + 1 - curr_seqnum);
    pkt_set_seqnum(pkt, curr_seqnum);
    pkt_set_timestamp(pkt, time(NULL));
    pkt_set_payload(pkt, data, len);
    return send_pkt(pkt);
}




