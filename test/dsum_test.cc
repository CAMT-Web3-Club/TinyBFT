#include "dsum.h"

#include <gtest/gtest.h>

static char data[4] = {'a', 'b', 'c', 'd'};

class DSumTest : public ::testing::Test {
 protected:
  void SetUp() override {
    libbyzea::DSum::init(
        "d2a10a09a80bc599b4d60bbec06c05d5e9f9c369954940145b63a1e2");
  }
};

TEST_F(DSumTest, AddSubEqual) {
  libbyzea::Digest digest(&data[0], 4);
  libbyzea::DSum got;
  libbyzea::DSum want = got;

  got.add(digest);
  got.sub(digest);
  EXPECT_EQ(mbedtls_mpi_cmp_mpi(&got.sum, &want.sum), 0);
}

TEST_F(DSumTest, SubAddEqual) {
  libbyzea::Digest digest(&data[0], 4);
  libbyzea::DSum got;
  libbyzea::DSum want = got;

  got.sub(digest);
  got.add(digest);

  EXPECT_EQ(mbedtls_mpi_cmp_mpi(&got.sum, &want.sum), 0);
}
