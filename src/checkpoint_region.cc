#include "checkpoint_region.h"

#include <cstring>
#include <new>

#include "Checkpoint.h"
#include "Replica.h"
#include "th_assert.h"

namespace libbyzea {
namespace checkpoint_region {

union Block {
  Checkpoint_rep msg;
  char raw[sizeof(Checkpoint_rep) + AUTHENTICATOR_SIZE];

  Block() { new (&msg) Checkpoint_rep(); }

  ~Block() {}
} __attribute__((aligned(ALIGNMENT)));

struct Certificate {
  Block checkpoints_[MAX_NUM_REPLICAS];
};

static constexpr size_t NUM_CERTS = max_out / checkpoint_interval + 1;
static Certificate checkpoint_certs[NUM_CERTS];
static Block above_window_checkpoints[MAX_NUM_REPLICAS - 1];
static Seqno head = -1;
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

Checkpoint_rep *load_checkpoint(Seqno seqno, size_t i) {
  th_assert(within_range(seqno) || seqno > max_seqno(),
            "Sequence number not in range");
  if (!within_range(seqno)) {
    auto &msg = above_window_checkpoints[replica_index(i)].msg;
    th_assert(seqno == msg.seqno, "Invalid state");
    return &msg;
  }

  size_t cert = certificate_index(seqno);
  return &checkpoint_certs[cert].checkpoints_[i].msg;
}

void store_checkpoint(Checkpoint_rep *checkpoint, size_t i) {
  th_assert(within_range(checkpoint->seqno) || checkpoint->seqno > max_seqno(),
            "Sequence number not in range");
  th_assert((size_t)checkpoint->size <= sizeof(Block),
            "Checkpoint message is too large");

  char *store;
  if (within_range(checkpoint->seqno)) {
    size_t cert_index = certificate_index(checkpoint->seqno);
    store = checkpoint_certs[cert_index].checkpoints_[i].raw;
  } else {
    th_assert(checkpoint->seqno >
                  above_window_checkpoints[replica_index(i)].msg.seqno,
              "Invalid state");
    store = above_window_checkpoints[replica_index(i)].raw;
  }
  std::memcpy(store, checkpoint, checkpoint->size);
}

void truncate(Seqno new_head) {
  if (new_head <= head_index) {
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
