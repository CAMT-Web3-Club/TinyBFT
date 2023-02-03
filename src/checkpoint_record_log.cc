#include "checkpoint_record_log.h"

#include "th_assert.h"

namespace libbyzea {
CheckpointRecordLog::CheckpointRecordLog(MEM_STATS_PARAM int num_state_blocks,
                                         Seqno head)
    : head_(head), fetch_seqno_(-1), head_index_(0), records_() {
  for (size_t i = 0; i < MAX_SIZE; i++) {
    records_[i].init(MEM_STATS_ARG_PUSH(CheckpointRecord) num_state_blocks);
  }

  MEM_STATS_GUARD_POP();
}

CheckpointRecordLog::~CheckpointRecordLog() {}

void CheckpointRecordLog::clear(Seqno h) {
  for (size_t i = 0; i < MAX_SIZE; i++) {
    records_[i].clear();
  }

  head_ = h;
  head_index_ = 0;
}

CheckpointRecord &CheckpointRecordLog::fetch(Seqno seqno) {
  th_assert(within_range(seqno), "Invalid state");

  if (seqno % checkpoint_interval != 0) {
    th_assert(fetch_seqno_ == seqno, "Invalid state");
    return records_[MAX_SIZE - 1];
  }
  return records_[record_index(seqno)];
}

void CheckpointRecordLog::truncate(Seqno new_head) {
  th_assert(new_head % checkpoint_interval == 0,
            "New head sequence number is not a valid checkpoint");
  if (new_head <= head_) {
    return;
  }
  // If the new head is in out of our existing window, we can simply clear the
  // whole log.
  if (!within_range(new_head)) {
    clear(new_head);
    return;
  }

  for (Seqno checkpoint = head_; checkpoint < new_head;
       checkpoint += checkpoint_interval) {
    records_[record_index(checkpoint)].clear();
  }
  if (fetch_seqno_ < new_head) {
    records_[MAX_SIZE - 1].clear();
  }

  head_index_ = record_index(new_head);
  head_ = new_head;
}

void CheckpointRecordLog::set_fetch_seqno(Seqno seqno) {
  if (fetch_seqno_ != -1) {
    records_[MAX_SIZE - 1].clear();
  }
  fetch_seqno_ = seqno;
}

}  // namespace libbyzea
