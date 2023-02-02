#ifndef LIBBYZEA_CHECKPOINT_LOG_H_
#define LIBBYZEA_CHECKPOINT_LOG_H_

#include "Certificate.h"
#include "Checkpoint.h"
#include "Time.h"
#include "mem_statistics_guard.h"

namespace libbyzea {

class CheckpointLog {
 public:
  CheckpointLog(MEM_STATS_REF);
  CheckpointLog(const CheckpointLog &) = delete;
  CheckpointLog(const CheckpointLog &&) = delete;

  ~CheckpointLog() = default;

  CheckpointLog &operator=(const CheckpointLog &) = delete;
  CheckpointLog &operator=(const CheckpointLog &&) = delete;

  void clear(Seqno h);
  // Effects: Calls "clear" for all elements in log and sets head to "h"

  Certificate<Checkpoint> &fetch(Seqno seqno);
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

  static constexpr size_t MAX_SIZE = max_out / checkpoint_interval + 1;

 private:
  /**
   * @brief Return seqno's certificate array index.
   *
   * Return the index index in the certificate array corresponding to sequence
   * number seqno.
   *
   * @param seqno the sequence number for which the index is computed.
   * @return int the index in the certificate array.
   */
  int certificate_index(Seqno seqno);

  Seqno head_;
  int head_index_;
  Certificate<Checkpoint> certs_[MAX_SIZE];
};

inline int CheckpointLog::certificate_index(Seqno seqno) {
  return (head_index_ + (seqno - head_) / checkpoint_interval) % MAX_SIZE;
}

inline bool CheckpointLog::within_range(Seqno seqno) const {
  return (seqno >= head_ && seqno <= max_seqno());
}

inline Seqno CheckpointLog::head_seqno() const { return head_; }

inline Seqno CheckpointLog::max_seqno() const { return head_ + max_out; }

}  // namespace libbyzea

#endif  // !LIBBYZEA_CHECKPOINT_LOG_H_
