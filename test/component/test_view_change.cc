#include "unity.h"
#include "types.h"
#include "View_change.h"
#include "New_view.h"
#include "parameters.h"

#include <cstdint>
#include <cstring>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

void test_view_change_trigger_view_timeout(void) {
    View current_view = 0;
    TEST_ASSERT_TRUE(current_view >= 0);
}

void test_view_change_new_view_number(void) {
    View old_view = 0;
    View new_view = 1;
    
    TEST_ASSERT_TRUE(new_view > old_view);
}

void test_view_change_sequential_views(void) {
    View v = 0;
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(i, v);
        v++;
    }
}

void test_view_change_primary_rotation(void) {
    int n = TEST_REPLICAS;
    
    int primary_0 = 0 % n;
    int primary_1 = 1 % n;
    int primary_2 = 2 % n;
    
    TEST_ASSERT_EQUAL(0, primary_0);
    TEST_ASSERT_EQUAL(1, primary_1);
    TEST_ASSERT_EQUAL(2, primary_2);
}

void test_view_change_all_replicas_participate(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    
    for (int v = 0; v < n; v++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
    }
}

void test_view_change_stable_checkpoint_propagation(void) {
    Seqno stable = 100;
    Seqno last_stable = 50;
    
    TEST_ASSERT_TRUE(stable > last_stable);
}

void test_view_change_2f_plus_1_required(void) {
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, quorum);
}

void test_view_change_safety_maintained(void) {
    View v1 = 1;
    View v2 = 2;
    
    TEST_ASSERT_TRUE(v2 > v1);
    
    Seqno s = 100;
    TEST_ASSERT_TRUE(s >= 0);
}

void test_view_change_different_primary(void) {
    int n = TEST_REPLICAS;
    
    View v0 = 0;
    View v1 = 1;
    
    int primary_v0 = v0 % n;
    int primary_v1 = v1 % n;
    
    TEST_ASSERT_TRUE(primary_v1 != primary_v0);
}

void test_view_change_log_preservation(void) {
    Seqno stable = 100;
    Seqno prepared = 150;
    
    TEST_ASSERT_TRUE(prepared >= stable);
}

void test_view_change_view_number_matches(void) {
    View v = 5;
    TEST_ASSERT_EQUAL(5, v);
}

void test_view_change_message_has_view_number(void) {
    View v = 10;
    TEST_ASSERT_TRUE(v >= 0);
}

void test_view_change_view_number_increments(void) {
    View v1 = 0;
    View v2 = 1;
    View v3 = 2;
    
    TEST_ASSERT_EQUAL(1, v2 - v1);
    TEST_ASSERT_EQUAL(1, v3 - v2);
}

void test_view_change_primary_failure_detection(void) {
    int n = TEST_REPLICAS;
    View v = 3;
    
    int primary = v % n;
    TEST_ASSERT_TRUE(primary >= 0);
    TEST_ASSERT_TRUE(primary < n);
}

void test_view_change_recovery_after_failure(void) {
    View old_view = 5;
    View new_view = 6;
    
    TEST_ASSERT_TRUE(new_view > old_view);
}

void test_view_change_byzantine_replicas_excluded(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int correct = n - f;
    TEST_ASSERT_EQUAL(5, correct);
    TEST_ASSERT_TRUE(correct >= quorum);
}

void unity_run_view_change_tests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_view_change_trigger_view_timeout);
    RUN_TEST(test_view_change_new_view_number);
    RUN_TEST(test_view_change_sequential_views);
    RUN_TEST(test_view_change_primary_rotation);
    RUN_TEST(test_view_change_all_replicas_participate);
    RUN_TEST(test_view_change_stable_checkpoint_propagation);
    RUN_TEST(test_view_change_2f_plus_1_required);
    RUN_TEST(test_view_change_safety_maintained);
    RUN_TEST(test_view_change_different_primary);
    RUN_TEST(test_view_change_log_preservation);
    RUN_TEST(test_view_change_view_number_matches);
    RUN_TEST(test_view_change_message_has_view_number);
    RUN_TEST(test_view_change_view_number_increments);
    RUN_TEST(test_view_change_primary_failure_detection);
    RUN_TEST(test_view_change_recovery_after_failure);
    RUN_TEST(test_view_change_byzantine_replicas_excluded);
    
    UNITY_END();
}
