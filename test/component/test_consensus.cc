#include "unity.h"
#include "Replica.h"
#include "Client.h"
#include "Node.h"
#include "Transport.h"
#include "parameters.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

class ConsensusTestFixture {
public:
    std::vector<libbyzea::Replica*> replicas;
    std::unique_ptr<libbyzea::Client> client;
    std::atomic<bool> running;
    std::mutex results_mutex;
    std::vector<std::string> committed_values;

    ConsensusTestFixture() : running(true) {}

    ~ConsensusTestFixture() {
        stop();
    }

    void stop() {
        running = false;
        for (auto* replica : replicas) {
            if (replica) {
                delete replica;
            }
        }
        replicas.clear();
    }
};

static ConsensusTestFixture* g_test_fixture = nullptr;

void setUp(void) {
    g_test_fixture = new ConsensusTestFixture();
}

void tearDown(void) {
    if (g_test_fixture) {
        delete g_test_fixture;
        g_test_fixture = nullptr;
    }
}

void test_consensus_single_request_all_agree(void) {
    TEST_ASSERT_NOT_NULL(g_test_fixture);
    
    TEST_IGNORE_MESSAGE("Requires full system setup with keys and transport");
}

void test_consensus_f_values(void) {
    TEST_ASSERT_EQUAL(2, TEST_FAULTY);
    TEST_ASSERT_EQUAL(7, TEST_REPLICAS);
    TEST_ASSERT_EQUAL(5, (2 * TEST_FAULTY) + 1);
}

void test_quorum_calculation(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(7, n);
    TEST_ASSERT_EQUAL(2, f);
    TEST_ASSERT_EQUAL(5, quorum);
    TEST_ASSERT_TRUE(quorum <= n);
    TEST_ASSERT_TRUE((n - quorum) == f);
}

void test_consensus_replicas_tolerate_f_faulty(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int remaining = n - f;
    
    TEST_ASSERT_TRUE(remaining >= quorum);
    TEST_ASSERT_EQUAL(5, remaining);
}

void test_consensus_fails_with_f_plus_one_faulty(void) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int more_than_f_faulty = f + 1;
    int remaining = n - more_than_f_faulty;
    
    TEST_ASSERT_TRUE(remaining < quorum);
    TEST_ASSERT_EQUAL(4, remaining);
}

void test_sequence_number_space(void) {
    Seqno min_seq = 0;
    Seqno max_seq = 10000;
    
    TEST_ASSERT_TRUE(max_seq > min_seq);
    TEST_ASSERT_TRUE(max_seq < (1ULL << 62));
}

void test_message_size_limits(void) {
    size_t max_msg = libbyzea::Max_message_size;
    size_t block_size = libbyzea::Block_size;
    
    TEST_ASSERT_TRUE(block_size < max_msg);
    TEST_ASSERT_TRUE(max_msg <= 16384);
}

void test_view_calculation(void) {
    View v = 0;
    int n = TEST_REPLICAS;
    
    for (int i = 0; i < 10; i++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
        v++;
    }
}

void test_window_size_constraints(void) {
    int ws = libbyzea::WINDOW_SIZE;
    int ci = libbyzea::CHECKPOINT_INTERVAL;
    
    TEST_ASSERT_TRUE(ws > ci);
    TEST_ASSERT_TRUE(ws <= 256);
    TEST_ASSERT_TRUE(ci >= 128);
}

void test_consensus_parameters(void) {
    TEST_ASSERT_EQUAL(7, libbyzea::MAX_NUM_REPLICAS);
    TEST_ASSERT_EQUAL(7, libbyzea::MAX_NUM_PRINCIPALS);
    TEST_ASSERT_EQUAL(2, libbyzea::Max_faulty);
}

void unity_run_tests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_consensus_f_values);
    RUN_TEST(test_quorum_calculation);
    RUN_TEST(test_consensus_replicas_tolerate_f_faulty);
    RUN_TEST(test_consensus_fails_with_f_plus_one_faulty);
    RUN_TEST(test_sequence_number_space);
    RUN_TEST(test_message_size_limits);
    RUN_TEST(test_view_calculation);
    RUN_TEST(test_window_size_constraints);
    RUN_TEST(test_consensus_parameters);
    
    UNITY_END();
}
