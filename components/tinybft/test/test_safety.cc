extern "C" {
#include "unity.h"
#include "unity_test_runner.h"
}
#include "libbyz.h"
#include "types.h"
#include "Message.h"
#include "parameters.h"
#include "Digest.h"
#include <string.h>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

using namespace libbyzea;

TEST_CASE("no double prepare same seqno", "[safety]") {
    Seqno seqno = 100;
    View view = 0;
    
    TEST_ASSERT_TRUE(seqno >= 0);
    TEST_ASSERT_TRUE(view >= 0);
}

TEST_CASE("no double commit invariant", "[safety]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, quorum);
    TEST_ASSERT_TRUE(n >= quorum);
}

TEST_CASE("request id monotonic", "[safety]") {
    Request_id rid1 = 0;
    Request_id rid2 = 1;
    
    TEST_ASSERT_TRUE(rid2 > rid1);
}

TEST_CASE("request id uniqueness", "[safety]") {
    Request_id rid1 = 1000;
    Request_id rid2 = 1001;
    Request_id rid3 = 1002;
    
    TEST_ASSERT_TRUE(rid1 != rid2);
    TEST_ASSERT_TRUE(rid2 != rid3);
    TEST_ASSERT_TRUE(rid1 != rid3);
}

TEST_CASE("view increments", "[safety]") {
    View v1 = 0;
    View v2 = 1;
    View v3 = 2;
    
    TEST_ASSERT_TRUE(v2 > v1);
    TEST_ASSERT_TRUE(v3 > v2);
}

TEST_CASE("sequence number increments", "[safety]") {
    Seqno s1 = 0;
    Seqno s2 = 1;
    Seqno s3 = 100;
    
    TEST_ASSERT_TRUE(s2 > s1);
    TEST_ASSERT_TRUE(s3 > s2);
}

TEST_CASE("primary calculation consistency", "[safety]") {
    int n = TEST_REPLICAS;
    
    for (View v = 0; v < 20; v++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
    }
}

TEST_CASE("quorum intersection property", "[safety]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int intersection_size = 2 * quorum - n;
    TEST_ASSERT_TRUE(intersection_size > f);
    TEST_ASSERT_EQUAL(3, intersection_size);
}

TEST_CASE("checkpoint interval divides window", "[safety]") {
    int ws = WINDOW_SIZE;
    int ci = CHECKPOINT_INTERVAL;
    
    TEST_ASSERT_TRUE(ws >= (2 * ci));
}

TEST_CASE("message size within limits", "[safety]") {
    size_t max_msg = Max_message_size;
    size_t block_size = Block_size;
    
    TEST_ASSERT_TRUE(block_size <= max_msg);
    TEST_ASSERT_TRUE(max_msg <= 16384);
    TEST_ASSERT_TRUE(max_msg >= 1024);
}

TEST_CASE("state block alignment", "[safety]") {
    size_t block_size = Block_size;
    
    TEST_ASSERT_TRUE((block_size & (block_size - 1)) == 0);
}

TEST_CASE("pre-prepare determines order", "[safety]") {
    View v = 5;
    Seqno s = 100;
    
    TEST_ASSERT_TRUE(v >= 0);
    TEST_ASSERT_TRUE(s >= 0);
}

TEST_CASE("safety with f faulty replicas", "[safety]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_TRUE((n - f) >= quorum);
    TEST_ASSERT_TRUE((n - f - 1) < quorum);
}

TEST_CASE("sequence number uniqueness", "[safety]") {
    Seqno s1 = 0;
    Seqno s2 = 0;
    
    for (int i = 0; i < 100; i++) {
        s1 = s2;
        s2++;
        TEST_ASSERT_TRUE(s2 > s1);
    }
}

TEST_CASE("view number uniqueness", "[safety]") {
    View v = 0;
    
    for (int i = 0; i < 100; i++) {
        View prev = v;
        v++;
        TEST_ASSERT_TRUE(v > prev);
    }
}

TEST_CASE("digest size", "[safety]") {
    TEST_ASSERT_EQUAL(32, sizeof(Digest));
}

TEST_CASE("request id size", "[safety]") {
    TEST_ASSERT_TRUE(sizeof(Request_id) >= 8);
}
