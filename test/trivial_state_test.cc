#include "trivial_state.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Digest.h"
#include "libbyz.h"

class TrivialStateTest : public ::testing::Test {
 public:
  TrivialStateTest() {
    std::memset(state_, 0x55, Block_size);
    libbyzea::Digest digest(state_, Block_size);
    want_digest_ = digest;
  }

 protected:
  char state_[Block_size];
  libbyzea::Digest want_digest_;
};

TEST_F(TrivialStateTest, RecoveryWithoutCheckpoints) {
  char name_template[26];
  strcpy(name_template, "trivial_state_test_XXXXXX");
  int fd = mkstemp(name_template);
  EXPECT_NE(fd, -1);

  libbyzea::TrivialState state(nullptr, state_, Block_size);
  FILE *wf = fdopen(fd, "w");
  bool ok = state.shutdown(wf, 0);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(fsync(fd), 0);

  FILE *rf = fdopen(fd, "r");
  ok = state.restart(rf, nullptr, 0, 0, false);
  EXPECT_EQ(ok, true);

  libbyzea::Digest got_digest(state_, Block_size);
  EXPECT_EQ(got_digest, want_digest_);
}
