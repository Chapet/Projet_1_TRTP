#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "sender.h"

int isUsefulAck_setup(void) {
    int i;
    int r = rand();
    for(i=0; i < r % BUFFER_SIZE; i++) {
        pkt_t * pkt = pkt_new();
        pkt_set_type(pkt, 1);
        pkt_set_tr(pkt, 0);
        pkt_set_window(pkt, 0);
        pkt_set_seqnum(pkt, i);
        char str[7];
        sprintf(str,"Payload no. %d", i);
        pkt_set_payload(pkt, str, 7);
        addToBuffer(pkt);
    }
    return 0;
}

int isUseful_teardown(void) {
    return 0;
}

void isUsefulAck_test(void) {
    CU_ASSERT
    return;
}


int main(int argc, char *argv[]) {
    printf("Tests are running ... \n");
    srand(time(NULL));

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    // Sender

    CU_pSuite pSenderSuite = NULL;
    pSenderSuite = CU_add_suite("ma_suite", sender_setup, sender_teardown);
    if (NULL == pSenderSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSenderSuite, "Test sender", sender_test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run tests
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());

    return EXIT_SUCCESS;
}