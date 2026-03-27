extern "C" {
#include "unity.h"
#include "unity_test_runner.h"
}
#include "Digest.h"
#include "hmac.h"
#include <string.h>

using namespace libbyzea;

TEST_CASE("Digest creation and verification", "[cryptography]") {
    const char *msg = "hello world";
    size_t len = strlen(msg);
    
    Digest d1(msg, len);
    Digest d2;
    d2.set(msg, len);
    
    TEST_ASSERT_TRUE(d1 == d2);
    TEST_ASSERT_FALSE(d1.is_zero());
    
    Digest d3("different", 9);
    TEST_ASSERT_TRUE(d1 != d3);
}

TEST_CASE("Digest copy and assignment", "[cryptography]") {
    const char *msg = "test data";
    Digest d1(msg, strlen(msg));
    
    Digest d2 = d1;
    TEST_ASSERT_TRUE(d1 == d2);
    
    Digest d3;
    d3 = d1;
    TEST_ASSERT_TRUE(d1 == d3);
}

TEST_CASE("HMAC basic test", "[cryptography]") {
    const char *key = "secret_key";
    const char *msg = "important message";
    
    Hmac h1(MBEDTLS_MD_SHA256, key, strlen(key));
    h1.update(msg, strlen(msg));
    
    char buf1[32];
    h1.sum(buf1);
    
    Hmac h2(MBEDTLS_MD_SHA256, key, strlen(key));
    h2.update(msg, strlen(msg));
    
    TEST_ASSERT_TRUE(h2.equals(buf1));
    
    // Different key should produce different HMAC
    Hmac h3(MBEDTLS_MD_SHA256, "wrong_key", 9);
    h3.update(msg, strlen(msg));
    TEST_ASSERT_FALSE(h3.equals(buf1));
}

TEST_CASE("HMAC reset and reuse", "[cryptography]") {
    const char *key = "key";
    Hmac h(MBEDTLS_MD_SHA256, key, strlen(key));
    
    h.update("data1", 5);
    char buf1[32];
    h.sum(buf1);
    
    h.reset();
    h.update("data1", 5);
    TEST_ASSERT_TRUE(h.equals(buf1));
}
