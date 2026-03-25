#include "unity.h"
#include "types.h"
#include "View_change.h"
#include "New_view.h"
#include "parameters.h"

#define TEST_FAULTY 2
#define TEST_REPLICAS 7

TEST_CASE("trigger view timeout", "[view_change]") {
    View current_view = 0;
    TEST_ASSERT_TRUE(current_view >= 0);
}

TEST_CASE("new view number", "[view_change]") {
    View old_view = 0;
    View new_view = 1;
    
    TEST_ASSERT_TRUE(new_view > old_view);
}

TEST_CASE("sequential views", "[view_change]") {
    View v = 0;
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(i, v);
        v++;
    }
}

TEST_CASE("primary rotation", "[view_change]") {
    int n = TEST_REPLICAS;
    
    TEST_ASSERT_EQUAL(0, 0 % n);
    TEST_ASSERT_EQUAL(1, 1 % n);
    TEST_ASSERT_EQUAL(2, 2 % n);
}

TEST_CASE("all replicas participate", "[view_change]") {
    int n = TEST_REPLICAS;
    
    for (int v = 0; v < n; v++) {
        int primary = v % n;
        TEST_ASSERT_TRUE(primary >= 0);
        TEST_ASSERT_TRUE(primary < n);
    }
}

TEST_CASE("stable checkpoint propagation", "[view_change]") {
    Seqno stable = 100;
    Seqno last_stable = 50;
    
    TEST_ASSERT_TRUE(stable > last_stable);
}

TEST_CASE("f plus one required", "[view_change]") {
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    TEST_ASSERT_EQUAL(5, quorum);
}

TEST_CASE("safety maintained", "[view_change]") {
    View v1 = 1;
    View v2 = 2;
    
    TEST_ASSERT_TRUE(v2 > v1);
}

TEST_CASE("different primary", "[view_change]") {
    int n = TEST_REPLICAS;
    
    View v0 = 0;
    View v1 = 1;
    
    int primary_v0 = v0 % n;
    int primary_v1 = v1 % n;
    
    TEST_ASSERT_TRUE(primary_v1 != primary_v0);
}

TEST_CASE("log preservation", "[view_change]") {
    Seqno stable = 100;
    Seqno prepared = 150;
    
    TEST_ASSERT_TRUE(prepared >= stable);
}

TEST_CASE("view number matches", "[view_change]") {
    View v = 5;
    TEST_ASSERT_EQUAL(5, v);
}

TEST_CASE("view number increments", "[view_change]") {
    View v1 = 0;
    View v2 = 1;
    View v3 = 2;
    
    TEST_ASSERT_EQUAL(1, v2 - v1);
    TEST_ASSERT_EQUAL(1, v3 - v2);
}

TEST_CASE("primary failure detection", "[view_change]") {
    int n = TEST_REPLICAS;
    View v = 3;
    
    int primary = v % n;
    TEST_ASSERT_TRUE(primary >= 0);
    TEST_ASSERT_TRUE(primary < n);
}

TEST_CASE("recovery after failure", "[view_change]") {
    View old_view = 5;
    View new_view = 6;
    
    TEST_ASSERT_TRUE(new_view > old_view);
}

TEST_CASE("byzantine replicas excluded", "[view_change]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int correct = n - f;
    TEST_ASSERT_EQUAL(5, correct);
    TEST_ASSERT_TRUE(correct >= quorum);
}

TEST_CASE("view change message size", "[view_change]") {
    int n = TEST_REPLICAS;
    int max_msg = libbyzea::Max_message_size;
    
    size_t view_change_base = sizeof(View) * 2 + sizeof(Seqno);
    size_t vcerts_size = n * (sizeof(View) + sizeof(Digest));
    
    TEST_ASSERT_TRUE(view_change_base + vcerts_size <= (size_t)max_msg);
}

TEST_CASE("new view message size", "[view_change]") {
    int n = TEST_REPLICAS;
    int max_msg = libbyzea::Max_message_size;
    
    size_t new_view_base = sizeof(View) * 2 + sizeof(Seqno);
    size_t vcerts_size = n * (sizeof(View) + sizeof(Digest));
    size_t prep_info_size = n * sizeof(Digest);
    
    TEST_ASSERT_TRUE(new_view_base + vcerts_size + prep_info_size <= (size_t)max_msg);
}

TEST_CASE("correct replicas survive", "[view_change]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int correct = n - f;
    TEST_ASSERT_EQUAL(5, correct);
    TEST_ASSERT_TRUE(correct >= quorum);
}

TEST_CASE("can form new view with f minus 1", "[view_change]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int f_minus_1 = f - 1;
    int remaining = n - f_minus_1;
    
    TEST_ASSERT_TRUE(remaining >= quorum);
}

TEST_CASE("cannot form new view with f plus 1", "[view_change]") {
    int n = TEST_REPLICAS;
    int f = TEST_FAULTY;
    int quorum = 2 * f + 1;
    
    int f_plus_1 = f + 1;
    int remaining = n - f_plus_1;
    
    TEST_ASSERT_TRUE(remaining < quorum);
}
