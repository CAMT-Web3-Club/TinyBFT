#ifndef _LIBBYZEA_TRIVIAL_STATE_H_
#define _LIBBYZEA_TRIVIAL_STATE_H_

#include "Data.h"
#include "Digest.h"
#include "Fetch.h"
#include "Log.h"
#include "Meta_data.h"
#include "Meta_data_cert.h"
#include "Meta_data_d.h"
#include "Time.h"
#include "checkpoint_record.h"
#include "types.h"

namespace libbyzea {

class Replica;

/// @brief Represents the application state and checkpoint records.
class TrivialState {
 public:
  TrivialState() = delete;
  TrivialState(Replica *replica, char *state, int state_len);

  TrivialState(const TrivialState &) = delete;
  TrivialState(TrivialState &&) = delete;

  TrivialState &operator=(const TrivialState &) = delete;
  TrivialState &operator=(TrivialState &&) = delete;

  ~TrivialState();

  void cow_single(int block_index);

  void cow(const char *start, int len);

  /// @brief Safe a checkpoint of the current state.
  ///
  /// Safe a checkpoint of t he current state and associate it with sequence
  /// number n.
  void checkpoint(Seqno n);

  /// @brief Roleback state to last checkpoint.
  ///
  /// Roles back the state to the last checkpoint and returns the checkpoint's
  /// sequence number.
  ///
  /// @return the sequence number associated with the checkpoint.
  Seqno rollback();

  void discard_checkpoint(Seqno up_to, Seqno current);

  void compute_full_digest();

  bool digest(Seqno n, Digest &digest);

  bool in_fetch_state() const;

  void start_fetch(Seqno last_exec, Seqno c = -1, Digest *c_digest = nullptr,
                   bool stable = false);

  void send_fetch(bool change_replier = false);

  bool in_check_state() const;

  void start_check(Seqno last_exec);

  void check_state();

  bool shutdown(FILE *output, Seqno ls);

  bool restart(FILE *input, Replica *replica, Seqno ls, Seqno le, bool corrupt);

  bool enforce_bound(Seqno bound, Seqno known_max_stable, bool corrupt);

  void handle(Meta_data *m);
  void handle(Meta_data_d *m);
  void handle(Data *m);

  bool handle(Fetch *m, Seqno last_stable);

  void mark_stale();

  void simulate_reboot();

  bool retrans_fetch(Time cur) const;

 private:
  Replica *replica_;

  bool fetching_;
  int last_replier_;
  Meta_data_cert *meta_data_cert_;
  Time last_fetch_time_;

  bool checking_;

  uint8_t *state_;
  size_t state_len_;

  Log<CheckpointRecord> checkpoint_log_;
  Seqno last_checkpoint_id_;
  bool keep_checkpoints_;
};

inline bool TrivialState::in_fetch_state() const { return fetching_; }

inline bool TrivialState::in_check_state() const { return checking_; }

inline bool TrivialState::retrans_fetch(Time cur) const {
  return fetching_ && diffTime(cur, last_fetch_time_) > 100000;
}
}  // namespace libbyzea
#endif  // !_LIBBYZEA_TRIVIAL_STATE_H_
