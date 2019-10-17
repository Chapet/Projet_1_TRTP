#ifndef FORMAT_SEGMENTS_SENDER_H
#define FORMAT_SEGMENTS_SENDER_H

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include "packet_interface.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>



typedef enum { // possible errors encountered in sender.c
    STATUS_OK = 0,
    E_GENERIC = 1,
    E_FILENAME,
    E_READING,
    E_CONNECT,
    E_TIMEOUT,
    E_SEND_PKT
} status_code;

typedef struct buffer {
    time_t timer;
    pkt_t * pkt;
    uint8_t seqnum;
} buffer_t;

char * hostname;
char * port;
struct addrinfo hints;
struct addrinfo * servinfo;
bool isSocketReady;

uint8_t curr_seqnum;
uint8_t window_end;
time_t retransmission_timer;
time_t deadlock_timeout; // 2min timeout if nothing is received and we can't send anything
struct timeval select_timeout;
buffer_t * sent_packets[32];
//uint8_t nbr_sent_packets=0;

int socket_fd;

status_code reader(char * filename);
status_code sender(char * buf, uint16_t len);
status_code send_pkt(pkt_t * pkt);
bool isToResend(uint8_t seqnum);
//int * whichToResend();
void removeFromSent(uint8_t seqnum);
void emptySocket();
void resendExpiredPkt();

#endif //FORMAT_SEGMENTS_SENDER_H
