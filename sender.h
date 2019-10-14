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

typedef enum { // possible errors encountered in sender.c
    STATUS_OK = 0,
    E_GENERIC = 1,
    E_FILENAME = 2,
    E_READING = 3,
    E_SENDER = 4,
} status_code;

char * hostname;
long port;

status_code reader(char * filename);
status_code sender(char * buf, uint16_t len);
#endif //FORMAT_SEGMENTS_SENDER_H
