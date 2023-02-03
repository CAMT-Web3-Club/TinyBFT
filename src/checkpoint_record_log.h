#ifndef _LIBBYZEA_CHECKPOINT_RECORD_LOG_H_
#define _LIBBYZEA_CHECKPOINT_RECORD_LOG_H_

#include "Partition.h"
#include "checkpoint_record.h"
#include "mem_statistics_guard.h"
#include "parameters.h"

namespace libbyzea {

class CheckpointRecordLog {
 public:
  // TODO: maybe + 2 due to fetch
  static constexpr size_t MAX_SIZE = max_out / checkpoint_interval + 2;

  CheckpointRecordLog(MEM_STATS_PARAM int num_state_blocks, Seqno head);

  CheckpointRecordLog(const CheckpointRecordLog &) = delete;
  CheckpointRecordLog(const CheckpointRecordLog &&) = delete;

  ~CheckpointRecordLog();

  CheckpointRecordLog &operator=(const CheckpointRecordLog &) = delete;
  CheckpointRecordLog &operator=(const CheckpointRecordLog &&) = delete;

  void clear(Seqno h);
  // Effects: Calls "clear" for all elements in log and sets head to "h"

  CheckpointRecord &fetch(Seqno seqno);
  // Requires: "within_range(seqno)"
  // Effects: Returns the entry corresponding to "seqno".

  void truncate(Seqno new_head);
  // Effects: Truncates the log clearing all elements with sequence
  // number lower than new_head.

  bool within_range(Seqno seqno) const;
  // Effects: Returns true iff "seqno" is within range.

  Seqno head_seqno() const;
  // Effects: Returns the sequence number for the head of the log.

  Seqno max_seqno() const;
  // Effects: Returns the maximum sequence number that can be
  // stored in the log.

  void set_fetch_seqno(Seqno seqno);

 private:
  int record_index(Seqno seqno) const;

  Seqno head_;
  Seqno fetch_seqno_;
  int head_index_;
  CheckpointRecord records_[MAX_SIZE];
};

inline int CheckpointRecordLog::record_index(Seqno seqno) const {
  if (seqno % checkpoint_interval != 0) {
    return MAX_SIZE - 1;
  }

  return (head_index_ + (seqno - head_) / checkpoint_interval) % (MAX_SIZE - 1);
}

inline bool CheckpointRecordLog::within_range(Seqno seqno) const {
  return (seqno >= head_ && seqno <= (head_ + max_out));
};

inline Seqno CheckpointRecordLog::head_seqno() const { return head_; };

inline Seqno CheckpointRecordLog::max_seqno() const { return head_ + max_out; };
}  // namespace libbyzea

#endif  // !_LIBBYZEA_CHECKPOINT_RECORD_LOG_H_
