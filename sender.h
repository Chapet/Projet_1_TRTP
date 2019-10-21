#ifndef FORMAT_SEGMENTS_SENDER_H
#define FORMAT_SEGMENTS_SENDER_H

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include "packet_interface.h"
#include <errno.h>
#include <poll.h>

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
    pkt_t *pkt;
    uint8_t seqnum;
} buffer_t;

char *hostname;
char *port;
struct addrinfo hints;
struct addrinfo *servinfo;
bool isSocketReady;
socklen_t addrlen;
struct sockaddr *dest_addr;

uint8_t curr_seqnum;
uint8_t window_end;
time_t retransmission_timer;
time_t deadlock_timeout; // 2min timeout if nothing is received and we can't send anything
struct timeval select_timeout;
buffer_t *sent_packets[31];
uint8_t recWindowFree;
uint8_t curr_ack_seqnum;

int socket_fd;

/*
 * Reads packet-sized data from the stdin or the file (if specified) until all the input has been "went trough"
 * after each read, the functions sends the packet trough the network to the specified destination with a call
 * to sender(). When all packets are sent, loop until the ack packets buffer is empty or a timeout occurs.
 *
 * @filename: the filename if specified, NULL otherwise.
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code reader(char *filename);

/*
 * If the socket and global variables are not ready yet initializes them.
 * Loop while the global timeout is not elapsed & the current sequence number is higher than the window end :
 * calls to emptySocket() & resendExpiredPkt() (getting acks and resending packets).
 * Then sends the pkt that was last read.
 *
 * @data: a pointer to a maximum of 512 bytes, containing the data to send over the network.
 * @len: the length of data.
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code sender(char *data, uint16_t len);

/*
 * Encodes the pkt, sends it on the socket, increments curr_seqnum and adds the pkt to the buffer sent_packets
 * (as a struct buffer_t).
 *
 * @pkt: the pkt to send on the socket.
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code send_pkt(pkt_t *pkt);

//int * whichToResend();

/*
 * Empties the socket from the received ack packets. If the pkt is of type ack, the corresponding element is removed
 * from the buffer sent_packets, if the pkt is of type nack, the corresponding pkt is resent.
 */
void emptySocket();

status_code addToBuffer(pkt_t *pkt);

/*
 * Checks the buffer for sent_pkt with the sequence number equal to seqnum.
 *
 * @ seqnum: a sequence number in the interval [0,255].
 *
 * @return: true if suck a packet is found, false otherwise.
 */
bool isToResend(uint8_t seqnum);

pkt_t * getFromBuffer(uint8_t seqnum);

/*
 * Checks the buffer for sent_pkt with the sequence number equal to seqnum, and if it is found, the pkt is
 * removed (freed).
 */
void removeFromSent(uint8_t seqnum);

/*
 * Checks for each element in the buffer packet_send if the retransmission is expired, if it is, the pkt is resent.
 */
void resendExpiredPkt();

//void printAsBinary(const char *buf, size_t len);

#endif //FORMAT_SEGMENTS_SENDER_H
