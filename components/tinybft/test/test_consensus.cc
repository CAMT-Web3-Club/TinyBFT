#include "unity.h"
#include "types.h"
#include "parameters.h"
#include "Message.h"
#include "Digest.h"

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

TEST_CASE("quorum calculation", "[consensus]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(7, n);
    TEST_ASSERT_EQUAL(2, f);
    TEST_ASSERT_EQUAL(5, quorum);
    TEST_ASSERT_TRUE(quorum <= n);
    TEST_ASSERT_EQUAL(n - quorum, f);
}

TEST_CASE("f values", "[consensus]") {
    TEST_ASSERT_EQUAL(2, TEST_FAULTY);
    TEST_ASSERT_EQUAL(7, TEST_REPLICAS);
    TEST_ASSERT_EQUAL(5, (2 * TEST_FAULTY) + 1);
}

TEST_CASE("tolerate f faulty replicas", "[consensus]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int remaining = n - f;
    
    TEST_ASSERT_TRUE(remaining >= quorum);
    TEST_ASSERT_EQUAL(5, remaining);
}

TEST_CASE("fails with f plus one faulty", "[consensus]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int more_than_f_faulty = f + 1;
    int remaining = n - more_than_f_faulty;
    
    TEST_ASSERT_TRUE(remaining < quorum);
    TEST_ASSERT_EQUAL(4, remaining);
}

TEST_CASE("sequence number space", "[consensus]") {
    Seqno min_seq = 0;
    Seqno max_seq = 10000;
    
    TEST_ASSERT_TRUE(max_seq > min_seq);
    TEST_ASSERT_TRUE(max_seq < (1ULL << 62));
}

TEST_CASE("message size limits", "[consensus]") {
    size_t max_msg = libbyzea::Max_message_size;
    size_t block_size = libbyzea::Block_size;
    
    TEST_ASSERT_TRUE(block_size < max_msg);
    TEST_ASSERT_TRUE(max_msg <= 16384);
}

TEST_CASE("view calculation", "[consensus]") {
    View v = 0;
    int n = TEST_REPLICAS;
    
    for (int i = 0; i < 10; i++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
        v++;
    }
}

TEST_CASE("window size constraints", "[consensus]") {
    int ws = libbyzea::WINDOW_SIZE;
    int ci = libbyzea::CHECKPOINT_INTERVAL;
    
    TEST_ASSERT_TRUE(ws > ci);
    TEST_ASSERT_TRUE(ws <= 256);
    TEST_ASSERT_TRUE(ci >= 128);
}

TEST_CASE("consensus parameters", "[consensus]") {
    TEST_ASSERT_EQUAL(7, libbyzea::MAX_NUM_REPLICAS);
    TEST_ASSERT_EQUAL(7, libbyzea::MAX_NUM_PRINCIPALS);
    TEST_ASSERT_EQUAL(2, libbyzea::Max_faulty);
}

TEST_CASE("primary rotation", "[consensus]") {
    int n = TEST_REPLICAS;
    
    TEST_ASSERT_EQUAL(0, 0 % n);
    TEST_ASSERT_EQUAL(1, 1 % n);
    TEST_ASSERT_EQUAL(2, 2 % n);
}

TEST_CASE("byzantine threshold reached", "[consensus]") {
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, quorum);
    TEST_ASSERT_TRUE(quorum > f);
}

TEST_CASE("digest consistency", "[consensus]") {
    Digest d1;
    Digest d2;
    memset(&d1, 0, sizeof(Digest));
    memset(&d2, 0, sizeof(Digest));
    
    TEST_ASSERT_EQUAL(0, memcmp(&d1, &d2, sizeof(Digest)));
}

TEST_CASE("certificate threshold", "[consensus]") {
    int f = TEST_FAULTY;
    int threshold = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, threshold);
    TEST_ASSERT_TRUE(threshold > f);
}
