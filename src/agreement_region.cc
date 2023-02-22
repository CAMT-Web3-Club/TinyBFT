#include "agreement_region.h"

#include <cstring>
#include <new>

#include "Commit.h"
#include "Log_allocator.h"
#include "Pre_prepare.h"
#include "Prepare.h"
#include "th_assert.h"

namespace libbyzea {

namespace agreement_region {

union PrePrepareBlock {
  Pre_prepare_rep msg;
  char raw[MAX_MESSAGE_SIZE];

  PrePrepareBlock() { new (&msg) Pre_prepare_rep(); }

  ~PrePrepareBlock() {}
} __attribute__((aligned(ALIGNMENT)));

union PrepareBlock {
  Prepare_rep msg;
  char raw[sizeof(Prepare_rep) + AUTHENTICATOR_SIZE];

  PrepareBlock() { new (&msg) Prepare_rep(); }

  ~PrepareBlock() {}
} __attribute__((aligned(ALIGNMENT)));
;

struct PreparedCert {
  PrePrepareBlock pre_prepare_;
  PrepareBlock prepares_[MAX_NUM_REPLICAS];
};

union CommitBlock {
  Commit_rep msg;
  char raw[sizeof(Commit_rep) + AUTHENTICATOR_SIZE];

  CommitBlock() { new (&msg) Commit_rep(); }
  ~CommitBlock() {}
} __attribute__((aligned(ALIGNMENT)));

struct CommitCert {
  CommitBlock commits_[MAX_NUM_REPLICAS];
};

struct AgreementSlice {
  PreparedCert prepared_cert;
  CommitCert commit_cert;
};

static AgreementSlice slices[max_out];
static Seqno head = 1;
static size_t head_index = 0;

size_t memory_demand() {
  return sizeof(slices) + sizeof(head) + sizeof(head_index);
}

static Seqno max_seqno() { return head + max_out - 1; }

static inline size_t cert_index(Seqno n) {
  return (head_index + (n - head)) % max_out;
}

Pre_prepare_rep *load_pre_prepare(Seqno n) {
  th_assert(within_range(n), "Sequence number not in range");
  return &slices[cert_index(n)].prepared_cert.pre_prepare_.msg;
}

void store_pre_prepare(Pre_prepare_rep *pre_prepare) {
  th_assert(within_range(pre_prepare->seqno),
            "Pre-Prepare sequence number not in range");
  th_assert((size_t)pre_prepare->size <= Max_message_size,
            "Pre-Prepare is too large");

  size_t i = cert_index(pre_prepare->seqno);
  auto *store = &slices[i].prepared_cert.pre_prepare_.raw;
  std::memcpy(store, pre_prepare, pre_prepare->size);
}

Prepare_rep *load_prepare(Seqno n, size_t i) {
  th_assert(within_range(n), "Sequence number out of range");
  th_assert(MAX_NUM_REPLICAS, "Prepare index out of range");
  return &slices[cert_index(n)].prepared_cert.prepares_[i].msg;
}

void store_prepare(Prepare_rep *prepare, size_t i) {
  th_assert(within_range(prepare->seqno), "Sequence number out of range");
  th_assert(MAX_NUM_REPLICAS, "Prepare index out of range");

  size_t sn = cert_index(prepare->seqno);
  auto *store = &slices[sn].prepared_cert.prepares_[i].raw;
  std::memcpy(store, prepare, prepare->size);
}

Commit_rep *load_commit(Seqno n, size_t i) {
  th_assert(within_range(n), "Sequence number out of range");
  th_assert(MAX_NUM_REPLICAS, "Prepare index out of range");
  return &slices[cert_index(n)].commit_cert.commits_[i].msg;
}

void store_commit(Commit_rep *commit, size_t i) {
  th_assert(within_range(commit->seqno), "Sequence number out of range");
  th_assert(MAX_NUM_REPLICAS, "Prepare index out of range");

  size_t sn = cert_index(commit->seqno);
  auto *store = &slices[sn].commit_cert.commits_[i].msg;
  std::memcpy(store, commit, commit->size);
}

bool within_range(Seqno seqno) {
  return (seqno >= head && seqno <= max_seqno());
}

void truncate(Seqno new_head) {
  if (new_head <= head) {
    return;
  }

  if (!within_range(new_head)) {
    head_index = 0;
    head = new_head;
    return;
  }

  head_index = new_head % max_out;
  head = new_head;
}

}  // namespace agreement_region

}  // namespace libbyzea
