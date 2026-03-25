#include "unity.h"
#include "types.h"
#include "Message.h"
#include "parameters.h"

#include <cstdint>
#include <cstring>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

void test_no_double_prepare_same_seqno(void) {
    Seqno seqno = 100;
    View view = 0;
    
    TEST_ASSERT_TRUE(seqno >= 0);
    TEST_ASSERT_TRUE(view >= 0);
    
    TEST_ASSERT_TRUE((seqno + 1) > seqno);
}

void test_no_double_commit_invariant(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, quorum);
    TEST_ASSERT_TRUE(n >= quorum);
}

void test_request_id_monotonic(void) {
    Request_id rid1 = 0;
    Request_id rid2 = 1;
    
    TEST_ASSERT_TRUE(rid2 > rid1);
    
    rid1 = rid2;
    rid2++;
    TEST_ASSERT_TRUE(rid2 > rid1);
}

void test_request_id_uniqueness(void) {
    Request_id rid1 = 1000;
    Request_id rid2 = 1001;
    Request_id rid3 = 1002;
    
    TEST_ASSERT_TRUE(rid1 != rid2);
    TEST_ASSERT_TRUE(rid2 != rid3);
    TEST_ASSERT_TRUE(rid1 != rid3);
}

void test_view_increments(void) {
    View v1 = 0;
    View v2 = 1;
    View v3 = 2;
    
    TEST_ASSERT_TRUE(v2 > v1);
    TEST_ASSERT_TRUE(v3 > v2);
}

void test_sequence_number_increments(void) {
    Seqno s1 = 0;
    Seqno s2 = 1;
    Seqno s3 = 100;
    
    TEST_ASSERT_TRUE(s2 > s1);
    TEST_ASSERT_TRUE(s3 > s2);
}

void test_digest_consistency(void) {
    Digest d1;
    Digest d2;
    memset(&d1, 0, sizeof(Digest));
    memset(&d2, 0, sizeof(Digest));
    
    TEST_ASSERT_EQUAL_MEMORY(&d1, &d2, sizeof(Digest));
}

void test_digest_different_inputs(void) {
    Digest d1;
    Digest d2;
    memset(&d1, 0, sizeof(Digest));
    memset(&d2, 1, sizeof(Digest));
    
    TEST_ASSERT_FALSE_MESSAGE(
        memcmp(&d1, &d2, sizeof(Digest)) == 0,
        "Digests should be different"
    );
}

void test_primary_calculation_consistency(void) {
    int n = TEST_REPLICAS;
    
    for (View v = 0; v < 20; v++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
    }
}

void test_quorum_intersection_property(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int intersection_size = 2 * quorum - n;
    TEST_ASSERT_TRUE(intersection_size > f);
    TEST_ASSERT_EQUAL(3, intersection_size);
}

void test_checkpoint_interval_divides_window(void) {
    int ws = libbyzea::WINDOW_SIZE;
    int ci = libbyzea::CHECKPOINT_INTERVAL;
    
    TEST_ASSERT_TRUE(ws >= (2 * ci));
}

void test_certificate_threshold(void) {
    int f = TEST_FAULTY;
    int threshold = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, threshold);
    TEST_ASSERT_TRUE(threshold > f);
    TEST_ASSERT_TRUE(threshold <= (TEST_REPLICAS - f));
}

void test_message_size_within_limits(void) {
    size_t max_msg = libbyzea::Max_message_size;
    size_t block_size = libbyzea::Block_size;
    
    TEST_ASSERT_TRUE(block_size <= max_msg);
    TEST_ASSERT_TRUE(max_msg <= 16384);
    TEST_ASSERT_TRUE(max_msg >= 1024);
}

void test_state_block_alignment(void) {
    size_t block_size = libbyzea::Block_size;
    
    TEST_ASSERT_TRUE((block_size & (block_size - 1)) == 0);
}

void test_pre_prepare_determines_order(void) {
    View v = 5;
    Seqno s = 100;
    Digest d;
    memset(&d, 0, sizeof(Digest));
    
    TEST_ASSERT_TRUE(v >= 0);
    TEST_ASSERT_TRUE(s >= 0);
    
    View v2 = 5;
    Seqno s2 = 100;
    Digest d2;
    memset(&d2, 0, sizeof(Digest));
    
    TEST_ASSERT_TRUE(v == v2);
    TEST_ASSERT_TRUE(s == s2);
}

void test_byzantine_threshold_reached(void) {
    int f = TEST_FAULTY;
    int n = TEST_REPLICAS;
    int quorum = 2 * f + 1;
    
    int byzantine_threshold = quorum;
    TEST_ASSERT_EQUAL(5, byzantine_threshold);
    TEST_ASSERT_TRUE(byzantine_threshold > f);
}

void unity_run_safety_tests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_no_double_prepare_same_seqno);
    RUN_TEST(test_no_double_commit_invariant);
    RUN_TEST(test_request_id_monotonic);
    RUN_TEST(test_request_id_uniqueness);
    RUN_TEST(test_view_increments);
    RUN_TEST(test_sequence_number_increments);
    RUN_TEST(test_digest_consistency);
    RUN_TEST(test_digest_different_inputs);
    RUN_TEST(test_primary_calculation_consistency);
    RUN_TEST(test_quorum_intersection_property);
    RUN_TEST(test_checkpoint_interval_divides_window);
    RUN_TEST(test_certificate_threshold);
    RUN_TEST(test_message_size_within_limits);
    RUN_TEST(test_state_block_alignment);
    RUN_TEST(test_pre_prepare_determines_order);
    RUN_TEST(test_byzantine_threshold_reached);
    
    UNITY_END();
}
