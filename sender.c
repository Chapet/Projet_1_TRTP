#include "sender.h"


status_code reader(char *filename) {

    // open filename if fArg, stdin otherwise
    int fd = STDIN_FILENO; // stdin (== 0)
    if (filename != NULL) { // arg -f not specified
        fd = open(filename, O_RDONLY); // opening the fd in read-only
        if (fd < 0) {
            return E_FILENAME;
        }
    }

    char buf[512]; // buffer of the data that is to become the payload
    isFinished = false;
    // side-note : there is no need to free an variable that wasn't obtained via malloc()/calloc()/realloc()
    status_code status;
    ssize_t nBytes = read(fd, &buf, 512); // the amount read by read() (0 = at or past EOF)

    // Read filename/stdin and send via sender
    while (nBytes > 0) {
        status = sender(buf, nBytes); // sender is a blocking call, so each time we read a pkt to send we send it
        if (status != STATUS_OK) {
            close(fd);
            return status;
        }
        nBytes = read(fd, &buf, 512); // reads again an a new value of nBytes is set
    } // all the packets have been sent, but maybe not received correctly

    time_t startEmptying = time(NULL);
    while (time(NULL) - startEmptying < deadlock_timeout && sent_packets[0] != NULL) {
        emptySocket();
        resendExpiredPkt();
    }

    isFinished = true;
    printf("Sending EOF packet \n");
    sender(NULL, 0);

    startEmptying = time(NULL);
    while (time(NULL) - startEmptying < deadlock_timeout && sent_packets[0] != NULL) {
        emptySocket();
        resendExpiredPkt();
    }

    close(fd);
    close(socket_fd);

    if (sent_packets[0] != NULL) {
        return E_TIMEOUT;
    }

    //checking for errors linked to the reading of the fd
    if (nBytes == 0) {
        return STATUS_OK;
    } else if (nBytes < 0) {
        return E_READING;
    } else {
        return E_GENERIC;
    }
}

bool isToResend(uint8_t seqnum) {
    int i;
    for (i = 0; i < 31 && sent_packets[i] != NULL; i++) {
        if (pkt_get_seqnum(sent_packets[i]->pkt) == seqnum) {
            return true;
        }
    }
    return false;
}

pkt_t * getFromBuffer(uint8_t seqnum) {
    int i;
    for (i=0; i < 31 && sent_packets[i] != NULL; i++) {
        if (pkt_get_seqnum(sent_packets[i]->pkt)== seqnum) {
            return sent_packets[i]->pkt;
        }
    }
    return NULL;
}

status_code addToBuffer(pkt_t * pkt) {
    buffer_t * sent_pkt = malloc(sizeof(buffer_t));
    if(sent_pkt == NULL) {
        return E_GENERIC;
    }
    sent_pkt->timer = time(NULL);
    sent_pkt->seqnum = pkt_get_seqnum(pkt);
    sent_pkt->pkt = pkt;
    // adds the packet that was just sent to the buffer and sets the different struct fields

    int i;
    for (i = 0; i < 31 && sent_packets[i] != NULL; i++); // set i to the correct index value
    sent_packets[i] = sent_pkt;

    return STATUS_OK;
}

status_code send_pkt(pkt_t *pkt) {
    size_t size =
            predict_header_length(pkt) + pkt_get_length(pkt) + 2 * sizeof(uint32_t); // header + CRC1 + payload + CRC2
    char *buf = malloc(size);
    if (buf == NULL) {
        return E_SEND_PKT;
    }
    pkt_status_code status = pkt_encode(pkt, buf, &size);
    if (status != PKT_OK) {
        return E_SEND_PKT;
    }
    //printAsBinary(buf, size);
    ssize_t sent = send(socket_fd, buf, size, 0);
    if (sent == -1) {
        printf("Ernno : %d <=> %s \n", errno, strerror(errno));
        return E_SEND_PKT;
    }

    free(buf);
    return STATUS_OK;
}

void removeFromSent(uint8_t seqnum) {
    int i;
    seqnum = seqnum - 1; //removing until seqnum EXCLUDED
    uint8_t nbShifted = 0;
    for(i=0; i < 31 && sent_packets[i] != NULL; i++) {
        // Sadly, not watching if an ack is relevant meaning between window start and end (cycling included)
        if((uint8_t)(sent_packets[i]->seqnum - seqnum) > 31u || sent_packets[i]->seqnum == seqnum) { // sent_packets[i]->seqnum < seqnum but handles uint8_t cycling
            printf("Deleting packet %d ..\n", sent_packets[i]->pkt->Seqnum);
            pkt_del(sent_packets[i]->pkt);
            free(sent_packets[i]);
            sent_packets[i] = NULL;
            nbShifted++;
        }
        if (nbShifted > 0) {
            if (sent_packets[i] == NULL) { // the element was deleted
                int j;
                for (j = 0; j < nbShifted - 1; j++) {
                    if (i + 1 + j >= 31) sent_packets[i - (nbShifted - 1) + j] = NULL;
                    else sent_packets[i - (nbShifted - 1) + j] = sent_packets[i + 1 + j];
                } // so we shift accordingly the preceding packets
            }
            // and in any case (element deleted or not) we replace the packet by the shifted value
            if (i + nbShifted >= 31) sent_packets[i] = NULL;
            else sent_packets[i] = sent_packets[i + nbShifted];
        }
    } // we found the element(s), deleted it(them) and shifted all the element after it to the left accordingly
}

void emptySocket() {
    /*
    fd_set read_fd;
    FD_ZERO(&read_fd); // clears the set
    FD_SET(socket_fd, &read_fd); // adds the fd socket_fd to the set
    int isAvailable = select(1, &read_fd, NULL, NULL, &select_timeout);
     */
    struct pollfd read_fd = {socket_fd, POLLIN, 0};
    int isAvailable = poll(&read_fd, 1, 10);

    char *buf = malloc(11); // the ack packets are 11 bytes long (7 header + 4 CRC)
    if (buf == NULL) {
        return;
    }
    pkt_t * pkt = pkt_new();
    while (isAvailable > 0 || recWindowFree == 0) {
        recv(socket_fd, buf, 11, 0); // rcv 11 bytes from the socket
        //printAsBinary(buf, 11);
        pkt_status_code status = pkt_decode(buf, 11, pkt);
        //pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)-1); // Supra-mystic line from nowhere => it works !!!!
        if ((uint8_t) (pkt_get_seqnum(pkt) - curr_ack_seqnum) >= 31u) {
            curr_ack_seqnum = pkt_get_seqnum(pkt);
            recWindowFree = pkt_get_window(pkt);
        }

        if (status == PKT_OK) {
            pkt_t *toResend = getFromBuffer(pkt->Seqnum);
            printf("Found in socket : PKT_TYPE %d for PKT %d\n", pkt->Type, pkt->Seqnum);
            if (pkt->Type == 2) { // pkt is PTYPE_ACK & is present in the sent_packet buffer
                //printf("Removing packet %d from sent_packets...\n", pkt->Seqnum);
                removeFromSent(pkt->Seqnum); // the pkt has been acked and thus removed from the sent_packet buffer
                if(isFinished && pkt->Seqnum == 0) {
                    removeFromSent(1);
                    return;
                }
                /*
                 * Could be implemented :
                 * watching number of occurrences of an ack in order to resend the corresponding pkt
                if(toResend != NULL) {
                    send_pkt(toResend);
                    printf("Packet %d resent ! (in emptySocket())\n", toResend->Seqnum);
                }
                 */
                printf("Packets until %d removed from sent_packets !\n", pkt->Seqnum);
            } else if (pkt->Type == 3 && toResend != NULL) { // pkt is PTYPE_NACK & is present in the sent_packet buffer
                //printf("Resending packet %d ...\n", pkt->Seqnum);
                send_pkt(toResend); // the packet is not acked, it is re-sent
                printf("Packet %d resent !\n", pkt->Seqnum);
            }
        }
        isAvailable = poll(&read_fd, 1, 1000);
    }
    pkt_del(pkt);
    free(buf);
}

void resendExpiredPkt() {
    int i;
    for(i=0; i < 31 && sent_packets[i] != NULL; i++) {
        if((time(NULL) - sent_packets[i]->timer) > retransmission_timer) {
            //printf("Resending packet %d ...\n", sent_packets[i]->pkt->Seqnum);
            printf("Packet %d has expired !\n", sent_packets[i]->pkt->Seqnum);
            send_pkt(sent_packets[i]->pkt);
        }
    }
}


status_code sender(char *data, uint16_t len) {

    if (!isSocketReady) { // the socket has not been initialized yet
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
        addrlen = servinfo->ai_addrlen;
        dest_addr = servinfo->ai_addr;


        if (connect(socket_fd, dest_addr, addrlen) == -1) {
            return E_CONNECT;
        }


        int i;
        for (i = 0; i < 31; i++) {
            sent_packets[i] = NULL;
        }
        curr_seqnum = 0;
        curr_ack_seqnum = 0;
        window_end = 30;
        recWindowFree = 1;
        retransmission_timer = 3;
        deadlock_timeout = 120; // 2 min timeout if nothing is received and we can't send anything
        isSocketReady = true;
    } // socket ready & global variables initialized

    emptySocket();
    resendExpiredPkt();
    time_t start = time(NULL);
    /* (window_end - curr_seqnum) > 31 : it just means that curr_seqnum > window_end, but handles the case where
     * curr_seqnum = window_end + 1 -> the difference is 255 (> 31)
     */
    while (time(NULL) - start < deadlock_timeout && (uint8_t)(window_end - curr_seqnum) > 31u && recWindowFree==0) {
        emptySocket();
        resendExpiredPkt();
    }


    if ((window_end - curr_seqnum) > 31) {
        return E_TIMEOUT;
    }

    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 0);
    pkt_set_seqnum(pkt, curr_seqnum);
    pkt_set_timestamp(pkt, time(NULL));
    pkt_set_payload(pkt, data, len);
    status_code status = send_pkt(pkt);
    if(status == STATUS_OK) {
        printf("Packet %d sent !\n", pkt->Seqnum);
        addToBuffer(pkt);
        curr_seqnum++;
    }
    return status;
}
/*
void printAsBinary(const char *buf, size_t len) {
    printf("Buf as binary : ");
    int i;
    for (i = 0; i < len; i++) {
        int j;
        for (j = 0; j < 8; j++) {
            uint8_t tmp = *(buf + i);
            tmp = tmp >> (7 - j);
            tmp = tmp & 0b00000001;
            if (tmp) printf("1");
            else printf("0");
        }
        printf(" ");
    }
    printf("\n");
}
*/
/*
 * implementer la window dynamique -> attention que quand on fait le tour/cycle, curr_seqnum > window_end peut être un pb
 *
 * tests
 */