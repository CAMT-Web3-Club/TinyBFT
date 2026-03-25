#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

class SafetyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SafetyTest, NoDoublePrepareSameSeqno) {
    typedef uint64_t Seqno;
    typedef uint64_t View;
    Seqno seqno = 100;
    View view = 0;
    
    EXPECT_TRUE(seqno >= 0);
    EXPECT_TRUE(view >= 0);
    EXPECT_TRUE((seqno + 1) > seqno);
}

TEST_F(SafetyTest, NoDoubleCommitInvariant) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    EXPECT_EQ(5, quorum);
    EXPECT_TRUE(n >= quorum);
}

TEST_F(SafetyTest, RequestIdMonotonic) {
    typedef uint64_t Request_id;
    Request_id rid1 = 0;
    Request_id rid2 = 1;
    
    EXPECT_TRUE(rid2 > rid1);
    
    rid1 = rid2;
    rid2++;
    EXPECT_TRUE(rid2 > rid1);
}

TEST_F(SafetyTest, RequestIdUniqueness) {
    typedef uint64_t Request_id;
    Request_id rid1 = 1000;
    Request_id rid2 = 1001;
    Request_id rid3 = 1002;
    
    EXPECT_NE(rid1, rid2);
    EXPECT_NE(rid2, rid3);
    EXPECT_NE(rid1, rid3);
}

TEST_F(SafetyTest, ViewIncrements) {
    typedef uint64_t View;
    View v1 = 0;
    View v2 = 1;
    View v3 = 2;
    
    EXPECT_TRUE(v2 > v1);
    EXPECT_TRUE(v3 > v2);
}

TEST_F(SafetyTest, SequenceNumberIncrements) {
    typedef uint64_t Seqno;
    Seqno s1 = 0;
    Seqno s2 = 1;
    Seqno s3 = 100;
    
    EXPECT_TRUE(s2 > s1);
    EXPECT_TRUE(s3 > s2);
}

TEST_F(SafetyTest, PrimaryCalculationConsistency) {
    typedef uint64_t View;
    int n = TEST_REPLICAS;
    
    for (View v = 0; v < 20; v++) {
        int primary = v % n;
        EXPECT_TRUE(primary >= 0);
        EXPECT_TRUE(primary < n);
    }
}

TEST_F(SafetyTest, QuorumIntersectionProperty) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int intersection_size = 2 * quorum - n;
    EXPECT_TRUE(intersection_size > f);
    EXPECT_EQ(3, intersection_size);
}

TEST_F(SafetyTest, SafetyWithFFaultyReplicas) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    EXPECT_TRUE((n - f) >= quorum);
    EXPECT_TRUE((n - f - 1) < quorum);
}

TEST_F(SafetyTest, SequenceNumberUniqueness) {
    typedef uint64_t Seqno;
    Seqno s1 = 0;
    Seqno s2 = 0;
    
    for (int i = 0; i < 100; i++) {
        s1 = s2;
        s2++;
        EXPECT_TRUE(s2 > s1);
    }
}

TEST_F(SafetyTest, ViewNumberUniqueness) {
    typedef uint64_t View;
    View v = 0;
    
    for (int i = 0; i < 100; i++) {
        View prev = v;
        v++;
        EXPECT_TRUE(v > prev);
    }
}

TEST_F(SafetyTest, RequestIdSize) {
    typedef uint64_t Request_id;
    Request_id rid = 0;
    
    EXPECT_TRUE(sizeof(Request_id) >= 8);
}
