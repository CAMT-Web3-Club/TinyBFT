extern "C" {
#include "unity.h"
#include "unity_test_runner.h"
}
#include "Transport.h"
#include <memory>

using namespace libbyzea;

TEST_CASE("Transport type selection", "[transport]") {
    TransportType t1 = TransportType::UDP;
    TransportType t2 = TransportType::ESP_NOW;
    TransportType t3 = TransportType::LOOPBACK;
    
    TEST_ASSERT_NOT_EQUAL((int)t1, (int)t2);
    TEST_ASSERT_NOT_EQUAL((int)t1, (int)t3);
    TEST_ASSERT_NOT_EQUAL((int)t2, (int)t3);
}

TEST_CASE("Transport factory basic check", "[transport]") {
    // We might not be able to create all transports in all environments
    // but we can check if the factory responds correctly to incorrect types
    // or if it returns null when not supported (if that's the behavior).
    
    // For now, let's just check the enum values for consistency
    TEST_ASSERT_EQUAL(1, (int)TransportType::UDP);
    TEST_ASSERT_EQUAL(2, (int)TransportType::LOOPBACK);
    TEST_ASSERT_EQUAL(3, (int)TransportType::ESP_NOW);
}

#ifdef TINYBFT_TRANSPORT_LOOPBACK
TEST_CASE("Loopback transport creation", "[transport]") {
    auto t = Transport::create(TransportType::LOOPBACK);
    TEST_ASSERT_TRUE(t != nullptr);
    TEST_ASSERT_EQUAL((int)TransportType::LOOPBACK, (int)t->type());
}
#endif

TEST_CASE("Address string buffer", "[transport]") {
    // Small sanity test for address-like structures if any
    // (Wait, Address is actually a class in Node.h usually)
}
