#include "checkpoint_log.h"

#include "th_assert.h"

namespace libbyzea {

CheckpointLog::CheckpointLog(MEM_STATS_REF)
    : head_(0), head_index_(0), certs_() {
  MEM_STATS_GUARD_POP();
}

void CheckpointLog::clear(Seqno h) {
  th_assert(h >= 0, "Checkpoint log head must be at least 0");
  for (size_t i = 0; i < MAX_SIZE; i++) {
    certs_[i].clear();
  }
  head_ = h;
}

Certificate<Checkpoint> &CheckpointLog::fetch(Seqno seqno) {
  th_assert(within_range(seqno), "Sequence number is out of bounds");
  th_assert(seqno % checkpoint_interval == 0,
            "Sequence number is not a valid checkpoint");

  return certs_[certificate_index(seqno)];
}

void CheckpointLog::truncate(Seqno new_head) {
  if (new_head <= head_) {
    return;
  }
  th_assert(new_head % checkpoint_interval == 0,
            "New head sequence number is not a valid checkpoint");

  // If the new head is in out of our existing window, we can simply clear the
  // whole log.
  if (!within_range(new_head)) {
    clear(new_head);
    return;
  }

  for (Seqno checkpoint = head_; checkpoint < new_head;
       checkpoint += checkpoint_interval) {
    certs_[certificate_index(checkpoint)].clear();
  }

  head_index_ = certificate_index(new_head);
  head_ = new_head;
}

}  // namespace libbyzea
