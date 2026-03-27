#include <stdio.h>
#include "unity.h"
#include "unity_test_runner.h"

void app_main(void) {
    UNITY_BEGIN();
    printf("Starting TinyBFT Unity Tests...\n");
    unity_run_all_tests();
    UNITY_END();
}
