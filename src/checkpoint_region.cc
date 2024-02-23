#include "checkpoint_region.h"

#include <cstring>
#include <new>

#include "Checkpoint.h"
#include "Message.h"
#include "Replica.h"
#include "th_assert.h"

namespace libbyzea {
namespace checkpoint_region {

constexpr auto CHECKPOINT_SIZE = sizeof(Checkpoint_rep) + AUTHENTICATOR_SIZE;

struct Block {
  Checkpoint checkpoint_;
  char msg_[CHECKPOINT_SIZE] __attribute__((aligned(ALIGNMENT)));

  Block() : checkpoint_(reinterpret_cast<Checkpoint_rep *>(msg_)) {}

  ~Block() {}
};

struct Certificate {
  Block checkpoints_[F + 1];
};

static constexpr size_t NUM_CERTS = max_out / checkpoint_interval + 1;
static Certificate checkpoint_certs[NUM_CERTS];
static Block above_window_checkpoints[MAX_NUM_REPLICAS - 1];
static Seqno head = 0;
static size_t head_index = 0;

size_t memory_demand() {
  return sizeof(NUM_CERTS) + sizeof(checkpoint_certs) + sizeof(head) +
         sizeof(head_index) + sizeof(above_window_checkpoints);
}

Seqno max_seqno() { return head + max_out; }

inline bool within_range(Seqno seqno) {
  return (seqno >= head && seqno <= max_seqno());
}

inline int certificate_index(Seqno seqno) {
  th_assert(within_range(seqno), "Sequence number not in range");
  return (head_index + (seqno - head) / checkpoint_interval) % NUM_CERTS;
}

static inline int replica_index(int replica_id) {
  return (replica_id <= replica->id()) ? replica_id : replica_id - 1;
}

Checkpoint *load_checkpoint(Seqno seqno, size_t i) {
  th_assert(within_range(seqno) || seqno > max_seqno(),
            "Sequence number not in range");
  if (!within_range(seqno)) {
    auto &msg = above_window_checkpoints[replica_index(i)].checkpoint_;
    th_assert(seqno == msg.seqno(), "Invalid state");
    return &msg;
  }

  size_t cert = certificate_index(seqno);
  return &checkpoint_certs[cert].checkpoints_[i].checkpoint_;
}

void store_checkpoint(Checkpoint *checkpoint, size_t i) {
  th_assert(checkpoint->seqno() >= head, "Sequence number not in range");
  th_assert((size_t)checkpoint->size() <= checkpoint_size,
            "Checkpoint message is too large");

  char *store;
  if (within_range(checkpoint->seqno())) {
    size_t cert_index = certificate_index(checkpoint->seqno());
    store = checkpoint_certs[cert_index].checkpoints_[i].msg_;
  } else {
    auto &cur = above_window_checkpoints[replica_index(i)].checkpoint_;
    th_assert(checkpoint->seqno() > cur.seqno(), "Invalid state");
    store = above_window_checkpoints[replica_index(i)].msg_;
  }
  std::memcpy(store, checkpoint->contents(), checkpoint->size());
}

void truncate(Seqno new_head) {
  if (new_head <= static_cast<Seqno>(head_index)) {
    return;
  }

  th_assert(new_head % checkpoint_interval == 0,
            "New head sequence number is not a valid checkpoint");
  if (!within_range(new_head)) {
    head_index = 0;
    head = new_head;
    return;
  }

  head_index = certificate_index(new_head);
  head = new_head;
}

}  // namespace checkpoint_region
}  // namespace libbyzea
