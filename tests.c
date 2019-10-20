#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "../src/sender.h"

int sender_setup(void) {
    return 0;
}

int sender_teardown(void) {
    return 0;
}

void sender_test(void) {
    return;
}


int main(int argc, char *argv[]) {
    printf("Tests are running ... \n");

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