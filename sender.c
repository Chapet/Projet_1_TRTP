#include <time.h>
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
        status = sender(buf, nBytes); // sender is a blocking call, so each time we read a pkt to send we send it
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
        if (pkt_get_seqnum(sent_packets[i]->pkt)== seqnum) {
            return true;
        }
    }
    return false;
}

status_code send_pkt(pkt_t * pkt) {
    size_t size = predict_header_length(pkt) + pkt_get_length(pkt) + 2* sizeof(uint32_t); // header + CRC1 + payload + CRC2
    char * buf = malloc(size);
    if (buf == NULL) {
        return E_SEND_PKT;
    }
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
    if(sent_pkt == NULL) {
        return E_SEND_PKT;
    }
    sent_pkt->timer = time(NULL);
    sent_pkt->seqnum = pkt_get_seqnum(pkt);
    sent_pkt->pkt = pkt;
    // adds the packet that was just sent to the buffer and sets the different struct fields

    int i;
    for(i=0; i < 31 && sent_packets[i] != NULL; i++); // set i to the correct index value
    sent_packets[i] = sent_pkt;

    return STATUS_OK;
}

void removeFromSent(uint8_t seqnum) { // doit aussi décaler tout les élements après
    int i;
    for(i=0; i < 31 && sent_packets[i] != NULL; i++) {
        if(sent_packets[i]->seqnum == seqnum) {
            free(sent_packets[i]->pkt->payload);
            free(sent_packets[i]->pkt);
            free(sent_packets[i]);
            sent_packets[i] = NULL;
        }
    }
}

void emptySocket() {
    fd_set read_fd;
    FD_ZERO(&read_fd); // clears the set
    FD_SET(socket_fd, &read_fd); // adds the fd socket_fd to the set

    int isAvailable = select(1, &read_fd, NULL, NULL, &select_timeout);
    // checks if the socket_fd can be read from, with a timeout of 1s

    char * buf = malloc(11); // the ack packets are 11 bytes long (7 header + 4 CRC)
    if (buf == NULL) {
        return;
    }

    while(isAvailable) { // while there is a ack packet to be read
        recv(socket_fd, buf, 11, 0); // rcv 11 bytes from the socket
        pkt_t * pkt = pkt_new();
        pkt_status_code status = pkt_decode(buf, 11, pkt);

        if (status == PKT_OK) {
            if (pkt->Type == 2 && isToResend(pkt->Seqnum)) { // pkt is PTYPE_ACK & is present in the sent_packet buffer
                removeFromSent(pkt->Seqnum); // the pkt has been acked and thus removed from the sent_packet buffer
            }
            else if (pkt->Type == 3 && isToResend(pkt->Seqnum)) { // pkt is PTYPE_NACK & is present in the sent_packet buffer
                send_pkt(pkt); // the packet is not acked, it is re-sent
            }
        }
        isAvailable = select(1, &read_fd, NULL, NULL, &select_timeout); // reset the select to look for incoming ack packets
    }

    free(buf);
}

void resendExpiredPkt() {
    int i;
    for(i=0; i < 31 && sent_packets[i] != NULL; i++) {
        if((time(NULL) - sent_packets[i]->timer) > retransmission_timer) {
            send_pkt(sent_packets[i]->pkt);
        }
    }
}


status_code sender(char * data, uint16_t len) {

    if(!isSocketReady) { // the socket has not been initialized yet
        socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            return E_CONNECT;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6; // IPv6
        hints.ai_socktype = SOCK_DGRAM; // diagram connectionless
        if (getaddrinfo(hostname, port, &hints, &servinfo) != 0) { //hostname : ipV6 address or hostname
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
        window_end=31;
        retransmission_timer = 5;
        deadlock_timeout = 120; // 2 min timeout if nothing is received and we can't send anything
        select_timeout = (struct timeval) {.tv_sec = 1, .tv_usec = 0}; // 1 s + 0 ms
        isSocketReady = true;
    } // socket ready & global variables initialized

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
     * implementer la window dynamique -> attention que quand on fait le tour curr_seqnum > window_end peut être un pb
     * décaler les stuct dans le buffer sent_packets
     * gérer les acks cumulatifs
     *
     * tests
     *
     * ligne 124 si la fonction c'est une void, je return en cas d'erreur dans le malloc ?
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




