#ifndef FORMAT_SEGMENTS_SENDER_H
#define FORMAT_SEGMENTS_SENDER_H

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include "packet_interface.h"

typedef enum {
    STATUS_OK = 0,
    E_GENERIC = 1,
    E_FILENAME = 2,
    E_READING = 3,
    E_SENDER = 4,
} status_code;

int opt;
bool fArg;
char * filename;
char * hostname;
long port;
bool test = false;

status_code reader();
status_code sender();
#endif //FORMAT_SEGMENTS_SENDER_H
