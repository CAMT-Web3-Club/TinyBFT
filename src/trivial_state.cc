#include "trivial_state.h"

#include <stdio.h>

#include <cstdlib>

#include "Fetch.h"
#include "Log.t"
#include "Node.h"
#include "th_assert.h"

namespace libbyzea {

TrivialState::TrivialState(Replica *replica, char *state, int state_len)
    : replica_(replica),
      fetching_(false),
      checking_(false),
      state_(reinterpret_cast<uint8_t *>(state)),
      state_len_(size_t(state_len)),
      checkpoint_log_(max_out * 2, 0),
      last_checkpoint_id_(0) {}

TrivialState::~TrivialState() {}

void TrivialState::cow_single([[maybe_unused]] int block_index) {}

void TrivialState::cow([[maybe_unused]] const char *start,
                       [[maybe_unused]] int len) {}

void TrivialState::checkpoint(Seqno n) {
  INCR_OP(num_ckpts);
  START_CC(ckpt_cycles);

  last_checkpoint_id_ = n;
  auto &record = checkpoint_log_.fetch(n);
  record.snapshot(state_, state_len_);

  STOP_CC(ckpt_cycles);
}

Seqno TrivialState::rollback() {
  th_assert(last_checkpoint_id_ >= 0 && !fetching_, "Invalid state");

  INCR_OP(num_rollbacks);
  START_CC(rollback_cycles);

  auto &record = checkpoint_log_.fetch(last_checkpoint_id_);
  record.copy(state_, state_len_);

  return last_checkpoint_id_;
}

void TrivialState::discard_checkpoint(Seqno up_to, Seqno current) {
  if (last_checkpoint_id_ >= 0 && up_to > last_checkpoint_id_ &&
      current > last_checkpoint_id_) {
    checkpoint(current);
    last_checkpoint_id_ = -1;
  }
  checkpoint_log_.truncate(up_to);
}

void TrivialState::compute_full_digest() {
  Cycle_counter cc;
  cc.start();
  checkpoint_log_.fetch(0).clear();
  checkpoint(0);
  cc.stop();

  printf("Compute full digest elapsed %qd\n", cc.elapsed());
}

bool TrivialState::digest(Seqno n, Digest &digest) {
  if (!checkpoint_log_.within_range(n)) {
    return false;
  }

  auto &record = checkpoint_log_.fetch(n);
  if (record.is_cleared()) {
    return false;
  }
  record.digest(digest);

  return true;
}

void TrivialState::start_fetch([[maybe_unused]] Seqno last_exec,
                               [[maybe_unused]] Seqno checkpoint,
                               [[maybe_unused]] Digest *checkpoint_digest,
                               [[maybe_unused]] bool stable) {}

void TrivialState::send_fetch([[maybe_unused]] bool change_replier) {}

void TrivialState::start_check([[maybe_unused]] Seqno last_exec) {}

void TrivialState::check_state() {}

bool TrivialState::shutdown([[maybe_unused]] FILE *output,
                            [[maybe_unused]] Seqno ls) {
  return true;
}

bool TrivialState::restart([[maybe_unused]] FILE *input,
                           [[maybe_unused]] Replica *replica,
                           [[maybe_unused]] Seqno ls, [[maybe_unused]] Seqno le,
                           [[maybe_unused]] bool corrupt) {
  return true;
}

bool TrivialState::enforce_bound([[maybe_unused]] Seqno bound,
                                 [[maybe_unused]] Seqno known_max_stable,
                                 [[maybe_unused]] bool corrupt) {
  return true;
}

void TrivialState::handle(Meta_data *m) { delete m; }
void TrivialState::handle(Meta_data_d *m) { delete m; }
void TrivialState::handle(Data *m) { delete m; }

bool TrivialState::handle(Fetch *m, [[maybe_unused]] Seqno last_stable) {
  delete m;
  return true;
}

void TrivialState::mark_stale() {}

void TrivialState::simulate_reboot() {}
}  // namespace libbyzea
