#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "sender.h"
int acks;

int isUsefulAck_setup(void) {
    int i;
    acks = rand() % BUFFER_SIZE+1;
    for(i=0; i <= acks; i++) {
        pkt_t * pkt = pkt_new();
        pkt_set_type(pkt, 1);
        pkt_set_tr(pkt, 0);
        pkt_set_window(pkt, 0);
        pkt_set_seqnum(pkt, i);
        char str[14];
        sprintf(str,"Payload no. %d", i);
        pkt_set_payload(pkt, str, 7);
        addToBuffer(pkt);
    }
    return 0;
}

int isUsefulAck_teardown(void) {
    int i;
    for(i=0; i<BUFFER_SIZE && sent_packets[i] != NULL; i++) {
        pkt_del(sent_packets[i]->pkt);
        free(sent_packets[i]);
        sent_packets[i] = NULL;
    }
    return 0;
}

void isUsefulAck_test(void) {
    int i;
    for(i=0; i <= acks; i++) {
        CU_ASSERT_EQUAL(isUsefulAck(i), true);
    }
    if (acks+1 < 2*BUFFER_SIZE) {
        for(i=acks+1; i < BUFFER_SIZE; i++) {
            CU_ASSERT_EQUAL(isUsefulAck(i), false);
        }
    }
    return;
}


int main(int argc, char *argv[]) {
    printf("Tests are running ... \n");
    srand(time(NULL));

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    // Sender

    CU_pSuite pIsUsefulAckSuite = NULL;
    pIsUsefulAckSuite = CU_add_suite("isUsefulAck_suite", isUsefulAck_setup, isUsefulAck_teardown);
    if (NULL == pIsUsefulAckSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pIsUsefulAckSuite, "isUsefulAck()'s Test", isUsefulAck_test) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run tests
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());

    return EXIT_SUCCESS;
}