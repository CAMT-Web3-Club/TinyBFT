#include "agreement_region.h"

#include <cstring>
#include <new>

#include "Commit.h"
#include "Log_allocator.h"
#include "Pre_prepare.h"
#include "Prepare.h"
#include "Req_queue.h"
#include "th_assert.h"
#include "types.h"

namespace libbyzea {

namespace agreement_region {

struct PrePrepareBlock {
  Pre_prepare pre_prepare_;
  char msg_[MAX_MESSAGE_SIZE] __attribute__((aligned(ALIGNMENT)));

  PrePrepareBlock() : pre_prepare_(reinterpret_cast<Pre_prepare_rep *>(msg_)) {}

  ~PrePrepareBlock() {}
};

struct PrepareBlock {
  Prepare prepare_;
  char msg_[sizeof(Prepare_rep) + AUTHENTICATOR_SIZE]
      __attribute__((aligned(ALIGNMENT)));

  PrepareBlock() : prepare_(reinterpret_cast<Prepare_rep *>(msg_)) {}

  ~PrepareBlock() {}
};

struct PreparedCert {
  PrePrepareBlock pre_prepare_block_;
  PrepareBlock prepares_[F + 1];

  PreparedCert() : pre_prepare_block_(), prepares_() {}
};

struct CommitBlock {
  Commit commit_;
  char msg_[sizeof(Commit_rep) + AUTHENTICATOR_SIZE]
      __attribute__((aligned(ALIGNMENT)));

  CommitBlock() : commit_(reinterpret_cast<Commit_rep *>(msg_)) {}
  ~CommitBlock() {}
} __attribute__((aligned(ALIGNMENT)));

struct CommitCert {
  CommitBlock commits_[F + 1];

  CommitCert() : commits_() {}
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

Pre_prepare *new_pre_prepare(View v, Seqno n, Req_queue &requests) {
  auto &b = slices[cert_index(n)].prepared_cert.pre_prepare_block_;
  auto pp = new (&b.pre_prepare_)
      Pre_prepare(reinterpret_cast<Pre_prepare_rep *>(b.msg_), v, n, requests);
  return pp;
}

Pre_prepare *load_pre_prepare(Seqno n) {
  th_assert(within_range(n), "Sequence number not in range");
  return &slices[cert_index(n)].prepared_cert.pre_prepare_block_.pre_prepare_;
}

void store_pre_prepare(Pre_prepare *pre_prepare) {
  th_assert(within_range(pre_prepare->seqno()),
            "Pre-Prepare sequence number not in range");
  th_assert((size_t)pre_prepare->size() <= Max_message_size,
            "Pre-Prepare is too large");

  size_t i = cert_index(pre_prepare->seqno());
  auto *store = &slices[i].prepared_cert.pre_prepare_block_.msg_;
  std::memcpy(store, pre_prepare->contents(), pre_prepare->size());
}

Prepare *load_prepare(Seqno n, size_t i) {
  th_assert(within_range(n), "Sequence number out of range");
  th_assert(i < (F + 1), "Prepare index out of range");
  return &slices[cert_index(n)].prepared_cert.prepares_[i].prepare_;
}

void store_prepare(Prepare *prepare, size_t i) {
  th_assert(within_range(prepare->seqno()), "Sequence number out of range");
  th_assert(i < (F + 1), "Prepare index out of range");

  size_t sn = cert_index(prepare->seqno());
  auto *store = &slices[sn].prepared_cert.prepares_[i].msg_;
  std::memcpy(store, prepare->contents(), prepare->size());
}

Commit *load_commit(Seqno n, size_t i) {
  th_assert(within_range(n), "Sequence number out of range");
  th_assert(i < (F + 1), "Prepare index out of range");
  return &slices[cert_index(n)].commit_cert.commits_[i].commit_;
}

void store_commit(Commit *commit, size_t i) {
  th_assert(within_range(commit->seqno()), "Sequence number out of range");
  th_assert(i < (F + 1), "Prepare index out of range");

  size_t sn = cert_index(commit->seqno());
  auto *store = &slices[sn].commit_cert.commits_[i].msg_;
  std::memcpy(store, commit->contents(), commit->size());
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
