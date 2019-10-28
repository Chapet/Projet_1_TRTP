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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include "packet_interface.h"
#include <errno.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 31 // size of the sending buffer

typedef enum { // possible errors encountered in sender.c
    STATUS_OK = 0,
    E_GENERIC = 1,
    E_FILENAME,
    E_CONNECT,
    E_TIMEOUT,
    E_SEND_PKT,
} status_code;

typedef struct buffer {
    time_t timer;
    pkt_t *pkt;
} buffer_t;

typedef struct counter {
    uint8_t ack_seq;
    int8_t occ;
} counter_t;

char *hostname;
char *port;
struct addrinfo hints;
struct addrinfo *servinfo;
socklen_t addrlen;
struct sockaddr *dest_addr;

uint8_t curr_seqnum; // the seqnum of the next packet to send
time_t retransmission_timer; // the retransmission timer for packets, init to 2 ms
time_t deadlock_timeout; // 30 sec timeout if nothing is received and we can't send anything
buffer_t *sent_packets[BUFFER_SIZE];
uint8_t nbElemBuf; // number of elements in the buffer sent_packets
uint8_t recWindowFree; // number of empty spaces in the receiver window
uint8_t toRemove; // number of element(s) to remove from the buffer sent_packets during removeFromSent()
uint8_t already_sent; // counts the number of pkt sent (new sent pkt, nack & retransmission)
int oldest_timestamp;
bool isFinished; // indicates if the program sent all the data (excluding the EOF-signaling packet)
counter_t fastRetrans; // used to check if the ack packets indicate a specific packet to retransmit
// (meaning he was probably not correctly received)



int socket_fd;
int file_fd;

/**
 * Frame of the program: it handles the calls to other functions. First calls init() then loops until all the input has
 * been read, sent as packets over the network to the destination indicated by filename, and received by the receiver.
 *
 * @filename: the filename if specified, NULL otherwise.
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 **/
status_code scheduler(char *filename);

/**
 * Creates and connects the socket to the specified receiver, initializes global variables.
 *
 * @filename: the filename if specified, NULL otherwise.
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code init(char *filename);

/**
 * Empties the socket from the received ack packets. If the pkt is of type ack, if the packet is relevant the variables
 * recWindowFree & already_sent are updated, and checks if we need quickly retransmit a packet. Else if the pkt is of
 * type nack, the corresponding pkt is resent.
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code emptySocket();

/**
 * Empties the socket from the received ack packets. If the pkt is of type ack, the corresponding element is removed
 * from the buffer sent_packets, if the pkt is of type nack, the corresponding pkt is resent.
 *
 * @seqnum : the sequence number of the packet to find in the buffer
 * @return: the packet with the sequence number specified, NULL otherwise
 */
pkt_t *getFromBuffer(uint8_t seqnum);

/**
 * Checks the buffer sent_pkt for the packet with the sequence number seqnum, and if it is found, the global variable
 * toRemove may be updated if the seqnum packet brought an information that was not already known (i.e. the sequence
 * number of the ack packet was higher than the ones from the previously received ack packets).
 *
 * @seqnum: the sequence number received in an ack packet
 * @return: true if the global variables toRemove & recWindowFree were updated, false otherwise
 */
bool isUsefulAck(uint8_t seqnum, uint32_t timestamp);

/**
 * Removes the (toRemove) first(s) packet(s) from the sent_packets buffer, and shifts the other elements accordingly.
 */
void removeFromSent();

/**
 * Checks for each element in the buffer packet_send if the retransmission is expired, if it is, the pkt is resent.
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code resendExpiredPkt();

/**
 * Encodes the pkt, sends it on the socket, increments already_sent and adds the pkt to the buffer sent_packets
 * (as a struct buffer_t).
 *
 * @pkt: a pointer to the pkt to send on the socket.
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code send_pkt(pkt_t *pkt);

/**
 * Creates a new packet and send it over the network. Afterwards, adds it to the buffer sent_packets and increment
 * curr_seqnum
 *
 * @data: a pointer to a maximum of 512 bytes, containing the data to send over the network.
 * @len: the length of data.
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code sender(char *data, uint16_t len);

/**
 * Adds the packet pkt to the buffer sent_packets, increments the variable nbElemBuf
 *
 * @pkt: a pointer to the pkt to add to the buffer sent_packets.
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code addToBuffer(pkt_t *pkt);

/**
 * Empties the buffer by looping on emptySocket(), removeFromSent() & resendExpiredPkt() while the buffer sent_packets
 * is not empty & a timeout is not reached
 *
 * @return: 0 (STATUS_OK) if no error occurred, the correct error status_code otherwise.
 */
status_code emptyBuffer();

/**
 * Closes the file_descriptors of the file & of the socket
 */
void close_fds();

#endif //FORMAT_SEGMENTS_SENDER_H
