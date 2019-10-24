#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "../src/sender.h"
int acks;

int sender_setup(void) {
    int i;
    acks = rand() % BUFFER_SIZE;
    nbElemBuf = 0;
    //acks=10;
    if(acks==0) acks++;
    for(i=0; i < acks; i++) {
        pkt_t * pkt = pkt_new();
        pkt_set_type(pkt, 1);
        pkt_set_tr(pkt, 0);
        pkt_set_window(pkt, 0);
        pkt_set_seqnum(pkt, i);
        char str[14];
        sprintf(str,"Payload no. %d", i);
        pkt_set_payload(pkt, str, 14);
        addToBuffer(pkt);
    }
    curr_seqnum = 0;
    best_expected = 0;
    toRemove=0;
    recWindowFree = 31;
    retransmission_timer = 2;
    already_sent = 0;
    deadlock_timeout = 30; // 2 min timeout if nothing is received and we can't send anything
    isSocketReady = true;
    isFinished = false;
    fastRetrans = (counter_t){0,0};
    return 0;
}

int sender_teardown(void) {
    int i;
    for(i=0; i<nbElemBuf && sent_packets[i] != NULL; i++) {
        pkt_del(sent_packets[i]->pkt);
        free(sent_packets[i]);
        sent_packets[i] = NULL;
    }
    return 0;
}

void addToBuffer_test(void) {
    pkt_t * pkt = pkt_new();
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 0);
    pkt_set_seqnum(pkt, 42);
    char str[14];
    sprintf(str,"Payload no. %d", acks+1);
    pkt_set_payload(pkt, str, 14);
    addToBuffer(pkt);
    CU_ASSERT_EQUAL(sent_packets[nbElemBuf-1]->pkt, pkt);
    //printf("%p == %p ?\n", sent_packets[nbElemBuf-1]->pkt,pkt);
    CU_ASSERT_EQUAL(sent_packets[nbElemBuf-1]->pkt->Seqnum, 42);
    pkt_del(sent_packets[nbElemBuf-1]->pkt);
    free(sent_packets[nbElemBuf-1]);
    sent_packets[nbElemBuf-1] = NULL;
    nbElemBuf--;
    return;
}

void isUsefulAck_test(void) {
    int i;
    for(i=0; i <= nbElemBuf; i++) {
        CU_ASSERT_EQUAL(isUsefulAck(i), true);
    }
    for(i=nbElemBuf+1; i < BUFFER_SIZE; i++) {
        CU_ASSERT_EQUAL(isUsefulAck(i), false);
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
        CU_ASSERT_EQUAL(time(NULL) - sent_packets[i]->timer < 2, true);
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

    // Sender

    CU_pSuite pSenderSuite = NULL;
    pSenderSuite = CU_add_suite("sender_suite", sender_setup, sender_teardown);
    if (NULL == pSenderSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /*
    if (NULL == CU_add_test(pIsUsefulAckSuite, "addToBuffer() Test", addToBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    */

    if (NULL == CU_add_test(pSenderSuite, "isUsefulAck() Test", isUsefulAck_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /*
    if (NULL == CU_add_test(pIsUsefulAckSuite, "getFromBuffer() Test", getFromBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pIsUsefulAckSuite, "resendExpiredPkt() Test", resendExpiredPkt_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pIsUsefulAckSuite, "removeFromSent() Test", removeFromSent_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pIsUsefulAckSuite, "emptyBuffer() Test", emptyBuffer_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
     */

    // Run tests
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());

    return EXIT_SUCCESS;
}