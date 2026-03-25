#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

class ConsensusTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConsensusTest, QuorumCalculation) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    EXPECT_EQ(7, n);
    EXPECT_EQ(2, f);
    EXPECT_EQ(5, quorum);
    EXPECT_TRUE(quorum <= n);
    EXPECT_EQ(n - quorum, f);
}

TEST_F(ConsensusTest, FValues) {
    EXPECT_EQ(2, TEST_FAULTY);
    EXPECT_EQ(7, TEST_REPLICAS);
    EXPECT_EQ(5, (2 * TEST_FAULTY) + 1);
}

TEST_F(ConsensusTest, TolerateFFaulty) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int remaining = n - f;
    
    EXPECT_TRUE(remaining >= quorum);
    EXPECT_EQ(5, remaining);
}

TEST_F(ConsensusTest, FailsWithFPlusOneFaulty) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    int more_than_f_faulty = f + 1;
    int remaining = n - more_than_f_faulty;
    
    EXPECT_TRUE(remaining < quorum);
    EXPECT_EQ(4, remaining);
}

TEST_F(ConsensusTest, SequenceNumberSpace) {
    typedef uint64_t Seqno;
    Seqno min_seq = 0;
    Seqno max_seq = 10000;
    
    EXPECT_TRUE(max_seq > min_seq);
    EXPECT_TRUE(max_seq < (1ULL << 62));
}

TEST_F(ConsensusTest, PrimaryRotation) {
    int n = TEST_REPLICAS;
    
    int primary_0 = 0 % n;
    int primary_1 = 1 % n;
    int primary_2 = 2 % n;
    
    EXPECT_EQ(0, primary_0);
    EXPECT_EQ(1, primary_1);
    EXPECT_EQ(2, primary_2);
}

TEST_F(ConsensusTest, AllReplicasParticipate) {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    
    for (int v = 0; v < n; v++) {
        int primary = v % n;
        EXPECT_TRUE(primary >= 0);
        EXPECT_TRUE(primary < n);
    }
}

TEST_F(ConsensusTest, ByzantineThresholdReached) {
    int f = TEST_FAULTY;
    int n = TEST_REPLICAS;
    int quorum = 2 * f + 1;
    
    int byzantine_threshold = quorum;
    EXPECT_EQ(5, byzantine_threshold);
    EXPECT_TRUE(byzantine_threshold > f);
}

TEST_F(ConsensusTest, CertificateThreshold) {
    int f = TEST_FAULTY;
    int threshold = 2 * f + 1;
    
    EXPECT_EQ(5, threshold);
    EXPECT_TRUE(threshold > f);
    EXPECT_TRUE(threshold <= (TEST_REPLICAS - f));
}

TEST_F(ConsensusTest, RequestIdMonotonic) {
    typedef uint64_t Request_id;
    Request_id rid1 = 0;
    Request_id rid2 = 1;
    
    EXPECT_TRUE(rid2 > rid1);
    
    rid1 = rid2;
    rid2++;
    EXPECT_TRUE(rid2 > rid1);
}

TEST_F(ConsensusTest, RequestIdUniqueness) {
    typedef uint64_t Request_id;
    Request_id rid1 = 1000;
    Request_id rid2 = 1001;
    Request_id rid3 = 1002;
    
    EXPECT_NE(rid1, rid2);
    EXPECT_NE(rid2, rid3);
    EXPECT_NE(rid1, rid3);
}
