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
    E_FILENAME = 2,
    E_READING = 3,
    E_SOCKET = 4,
    E_CONNECT = 5
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
bool isSocketReady=false;

uint8_t curr_seqnum=0;
uint8_t window_end=31;
time_t retransmission_timer = 5;
buffer_t sent_packets[32];

int socket_fd;

status_code reader(char * filename);
status_code sender(char * buf, uint16_t len);
#endif //FORMAT_SEGMENTS_SENDER_H
