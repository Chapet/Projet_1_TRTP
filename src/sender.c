#include "sender.h"


status_code scheduler(char *filename) {
//TODO : si receiver injoignable, sleep(10)
    status_code init_ret = init(filename);
    if (init_ret != STATUS_OK) return init_ret;
    // side-note : there is no need to free an variable that wasn't obtained via malloc()/calloc()/realloc()

    char buf[512];
    time_t start = time(NULL);
    while (!isFinished && time(NULL) - start < deadlock_timeout) {

        status_code status = emptySocket();
        if (status != STATUS_OK) {
            close_fds();
            return status;
        }
        removeFromSent();
        resendExpiredPkt();

        while ((recWindowFree - already_sent) != 0 && nbElemBuf < BUFFER_SIZE) {
            ssize_t nBytes = read(file_fd, &buf, 512); // the amount read by read() (0 = at or past EOF)
            // Read filename/stdin and send via sender
            if (nBytes > 0) {
                status = sender(buf,
                                nBytes); // sender is a blocking call, so each time we read a pkt to send we send it
                if (status != STATUS_OK) {
                    close_fds();
                    return status;
                }
            } // all the packets have been sent, but maybe not received correctly
            else {
                isFinished = true;
                status = emptyBuffer();
                if (status != STATUS_OK) {
                    return status;
                }
                sleep(2);
                status = sender(NULL, 0); // send the last pkt
                if (status != STATUS_OK) return status;
                break;
            }
            start = time(NULL);
        }
    }
    if (!isFinished) {
        return E_TIMEOUT;
    }
    sleep(2);
    return emptyBuffer();
}

status_code init(char *filename) {
    file_fd = STDIN_FILENO; // stdin (== 0)
    if (filename != NULL) { // arg -f not specified
        file_fd = open(filename, O_RDONLY); // opening the fd in read-only
        if (file_fd < 0) {
            return E_FILENAME;
        }
    }

    socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        close(file_fd);
        return E_CONNECT;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6; // IPv6
    hints.ai_socktype = SOCK_DGRAM; // diagram connectionless
    if (getaddrinfo(hostname, port, &hints, &servinfo) != 0) { //hostname : ipV6 address or hostname
        close(file_fd);
        return E_CONNECT;
    }

    // retrieving values from struct addrinfo
    addrlen = servinfo->ai_addrlen;
    dest_addr = servinfo->ai_addr;


    if (connect(socket_fd, dest_addr, addrlen) == -1) {
        close(file_fd);
        return E_CONNECT;
    }


    int i;
    for (i = 0; i < 31; i++) {
        sent_packets[i] = NULL;
    }
    curr_seqnum = 0;
    toRemove = 0;
    recWindowFree = 1;
    nbElemBuf = 0;
    retransmission_timer = 2;
    already_sent = 0;
    deadlock_timeout = 30; // 30 sec timeout if nothing is received and we can't send anything
    isFinished = false;
    fastRetrans = (counter_t) {0, 0};
    return STATUS_OK;
}

status_code emptySocket() {
    struct pollfd read_fd = {socket_fd, POLLIN, 0};
    int isAvailable = poll(&read_fd, 1, 10); // positive if there is something to read in the socket

    char *buf = malloc(11); // the ack packets are 11 bytes long (7 header + 4 CRC)
    if (buf == NULL) {
        return E_GENERIC;
    }
    pkt_t *pkt = pkt_new();

    while (isAvailable > 0) {
        recv(socket_fd, buf, 11, 0); // receive 11 bytes from the socket
        pkt_status_code status = pkt_decode(buf, 11, pkt);

        if (status == PKT_OK) {
            pkt_t *toResend = getFromBuffer(pkt->Seqnum);
            if (pkt->Type == 2) {
                if (isUsefulAck(pkt->Seqnum)) { // pkt is PTYPE_ACK & is present in the sent_packets buffer
                    recWindowFree = pkt->Window;
                    already_sent = 0;
                    if (fastRetrans.ack_seq == pkt->Seqnum) {
                        fastRetrans.occ++;
                    } else {
                        fastRetrans.ack_seq = pkt->Seqnum;
                    }
                    if (toResend != NULL && fastRetrans.occ > 2 && !isFinished) {
                        fastRetrans.occ = 0;
                        send_pkt(toResend);
                    }
                }
            } else if (pkt->Type == 3 &&
                       toResend != NULL) { // pkt is PTYPE_NACK & is present in the sent_packets buffer
                send_pkt(toResend); // the packet is not acked, it is re-sent
            }
        }
        isAvailable = poll(&read_fd, 1, 10);
    }
    pkt_del(pkt);
    free(buf);
    return STATUS_OK;
}

pkt_t *getFromBuffer(uint8_t seqnum) {
    int i;
    for (i = 0; i < 31 && sent_packets[i] != NULL; i++) {
        if (pkt_get_seqnum(sent_packets[i]->pkt) == seqnum) {
            return sent_packets[i]->pkt;
        }
    }
    return NULL;
}

bool isUsefulAck(uint8_t seqnum) {
    if (nbElemBuf == 0) {
        return false;
    }
    uint8_t tmp = (uint8_t) (seqnum - sent_packets[0]->pkt->Seqnum);
    if (tmp <= nbElemBuf) { // TODO : use the timestamp to check if the pkt is an old pkt
        if (tmp >= toRemove) {
            toRemove = tmp;
            return true;
        }
    }
    return false;
}

void removeFromSent() {
    if (toRemove > 0) {
        int i;
        for (i = 0; i < toRemove && sent_packets[i] != NULL; i++) {
            pkt_del(sent_packets[i]->pkt);
            free(sent_packets[i]);
            sent_packets[i] = NULL;
        }
        for (i = toRemove; i < nbElemBuf && sent_packets[i] != NULL; i++) {
            sent_packets[i - toRemove] = sent_packets[i];
            sent_packets[i] = NULL;
        }
        nbElemBuf -= toRemove;
        toRemove = 0;
    }
}

void resendExpiredPkt() {
    int i;
    for (i = 0; i < 31 && sent_packets[i] != NULL; i++) {
        if ((time(NULL) - sent_packets[i]->timer) > retransmission_timer) {
            sent_packets[i]->timer = time(NULL);
            send_pkt(sent_packets[i]->pkt);
            // TODO : We DO NOT check the return of send_pkt
        }
    }
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
    ssize_t sent = send(socket_fd, buf, size, 0);
    if (sent == -1) {
        printf("Ernno : %d (%s) -> You should ignore this if you have launched the tests\n", errno, strerror(errno));
        return E_SEND_PKT;
    }
    already_sent++;
    free(buf);
    return STATUS_OK;
}

status_code sender(char *data, uint16_t len) {
    pkt_t *pkt = pkt_new(); // creating a new empty packet
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 0);
    pkt_set_seqnum(pkt, curr_seqnum);
    pkt_set_timestamp(pkt, time(NULL));
    pkt_set_payload(pkt, data, len);
    status_code status = send_pkt(pkt);
    if (status == STATUS_OK) {
        addToBuffer(pkt);
        curr_seqnum++;
    }
    return status;
}

status_code addToBuffer(pkt_t *pkt) {
    buffer_t *sent_pkt = malloc(sizeof(buffer_t));
    if (sent_pkt == NULL) {
        return E_GENERIC;
    }
    sent_pkt->timer = time(NULL);
    sent_pkt->pkt = pkt;
    // adds the packet that was just sent to the buffer and sets the different struct fields

    int i;
    for (i = 0; i < BUFFER_SIZE && sent_packets[i] != NULL; i++); // set i to the correct index value
    sent_packets[i] = sent_pkt;

    nbElemBuf++;
    return STATUS_OK;
}

status_code emptyBuffer() {
    time_t startEmptying = time(NULL);
    while (time(NULL) - startEmptying < deadlock_timeout && sent_packets[0] != NULL) {
        emptySocket();
        removeFromSent();
        resendExpiredPkt();
    }
    if (sent_packets[0] != NULL) return E_TIMEOUT;
    else return STATUS_OK;
}

void close_fds() {
    close(file_fd);
    close(socket_fd);
}