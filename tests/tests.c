    #include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "../src/sender.h"
int log_fd;

int sender_setup(void) {
    log_fd = open("tests.log", O_TRUNC | O_CREAT | O_RDWR, S_IRWXU);
    if(log_fd==-1) {
        printf("Creation of file tests.log has failed : logs redirected to stdin\n");
        log_fd = STDIN_FILENO;
    }
    int i;
    int acks = rand() % BUFFER_SIZE-1;
    acks++;
    nbElemBuf = 0;
    for(i=0; i < acks; i++) {
        pkt_t * pkt = pkt_new();
        pkt_set_type(pkt, 1);
        pkt_set_tr(pkt, 0);
        pkt_set_window(pkt, 0);
        pkt_set_seqnum(pkt, i);
        pkt_set_payload(pkt, "Test payload", 13);
        addToBuffer(pkt);
    }

    curr_seqnum = acks;
    toRemove=0;
    ssize_t nBytes = write(log_fd, "Content of sent_packets :\n", 26);
    if(nBytes != 26) {
        printf("Error while writing the log file\n");
    }
    for(i=0; i<nbElemBuf; i++) {
        char buffer[42];
        snprintf(buffer, sizeof(buffer), "Packet %d in sent_packets with seqnum = %d\n", i, sent_packets[i]->pkt->Seqnum);
        nBytes = write(log_fd, buffer, 42);
        if(nBytes != 42) {
            printf("Error while writing the log file\n");
        }

    }
    return 0;
}

int sender_teardown(void) {
    int i;
    for(i=0; i<nbElemBuf && sent_packets[i] != NULL; i++) {
        pkt_del(sent_packets[i]->pkt);
        free(sent_packets[i]);
        sent_packets[i] = NULL;
    }
    close(log_fd);
    return 0;
}

void addToBuffer_test(void) {
    int i;
    int acks = rand() % BUFFER_SIZE;
    pkt_t * pkts[acks+1];
    nbElemBuf = 0;
    for(i=0; i <= acks; i++) {
        pkt_t * pkt = pkt_new();
        pkt_set_type(pkt, 1);
        pkt_set_tr(pkt, 0);
        pkt_set_window(pkt, 0);
        pkt_set_seqnum(pkt, i);
        pkt_set_payload(pkt, "Test payload", 13);
        pkts[i] = pkt;
        addToBuffer(pkt);
    }
    curr_seqnum = acks;
    toRemove=0;
    for(i=0; i <= acks; i++) {
        CU_ASSERT_EQUAL(sent_packets[i]->pkt, pkts[i]);
        CU_ASSERT_EQUAL(sent_packets[i]->pkt->Seqnum, i);
        pkt_del(sent_packets[i]->pkt);
        free(sent_packets[i]);
        sent_packets[i] = NULL;
        nbElemBuf--;
    }
    return;
}

void isUsefulAck_test(void) {
    if(nbElemBuf==0) {
        CU_ASSERT_TRUE(isUsefulAck(0, time(NULL)));
        printf("isUsefulAck with no element returned true !\n");
        return;
    }
    int i;
    for(i=0; i <= nbElemBuf; i++) {
        char buffer[54];
        bool ret = isUsefulAck(i, time(NULL));
        snprintf(buffer, 54, "isUsefulAck(i) with i = %d returned %d\n with expected 1",i,ret);
        int nBytes = write(log_fd, buffer, 54);
        if(nBytes != 54) {
            printf("Error while writing the log file\n");
        }
        CU_ASSERT_TRUE(ret);
    }
    for(i=nbElemBuf+1; i < BUFFER_SIZE; i++) {
        char buffer[54];
        bool ret = isUsefulAck(i, time(NULL));
        snprintf(buffer, 54,"isUsefulAck(i) with i = %d returned %d\n with expected 0",i,ret);
        int nBytes = write(log_fd, buffer, 54);
        if(nBytes != 54) {
            printf("Error while writing the log file\n");
        }
        CU_ASSERT_FALSE(ret);
    }
    return;
}

void getFromBuffer_test(void) {
    int i;
    for(i=0; i < nbElemBuf; i++) {
        CU_ASSERT_EQUAL(getFromBuffer(i), sent_packets[i]->pkt);
    }
    int j = nbElemBuf;
    if (j < 2*BUFFER_SIZE) {
        for(i=j; i < BUFFER_SIZE; i++) {
            CU_ASSERT_EQUAL(getFromBuffer(i), NULL);
        }
    }
    return;
}

void resendExpiredPkt_test(void) {
    resendExpiredPkt();
    int i;
    for (i = 0; i < 31 && sent_packets[i] != NULL; i++) {
        CU_ASSERT_TRUE(time(NULL) - sent_packets[i]->timer < 2);
    }
    return;
}

void removeFromSent_test(void) {
    toRemove=nbElemBuf;
    removeFromSent();
    int i;
    for(i=0; i<BUFFER_SIZE; i++) {
        CU_ASSERT_EQUAL(sent_packets[i], NULL);
    }
    CU_ASSERT_EQUAL(nbElemBuf, 0);
    CU_ASSERT_EQUAL(toRemove, 0);
    return;
}

void emptyBuffer_test(void) {
    sender_setup();
    toRemove=0;
    deadlock_timeout=3;
    // Errors are expected because the socket isn't connected.
    status_code status = emptyBuffer();
    CU_ASSERT_EQUAL(status, E_TIMEOUT);
    toRemove=nbElemBuf;
    deadlock_timeout=3;
    status = emptyBuffer();
    CU_ASSERT_EQUAL(status, STATUS_OK);
    return;
}




int main(int argc, char *argv[]) {
    printf("Tests are running ... \n");
    srand(time(NULL));

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite pAddToBufferSuite = NULL;
    pAddToBufferSuite = CU_add_suite("addToBuffer_suite", NULL, NULL);
    if (NULL == pAddToBufferSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pAddToBufferSuite, "addToBuffer() Test", addToBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_pSuite pSenderSuite = NULL;
    pSenderSuite = CU_add_suite("sender_suite", sender_setup, sender_teardown);
    if (NULL == pSenderSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }  

    if (NULL == CU_add_test(pSenderSuite, "isUsefulAck() Test", isUsefulAck_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    
    if (NULL == CU_add_test(pSenderSuite, "getFromBuffer() Test", getFromBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    if (NULL == CU_add_test(pSenderSuite, "resendExpiredPkt() Test", resendExpiredPkt_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSenderSuite, "removeFromSent() Test", removeFromSent_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /*
    if (NULL == CU_add_test(pSenderSuite, "emptyBuffer() Test", emptyBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    */
    

    // Run tests
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());

    return EXIT_SUCCESS;
}