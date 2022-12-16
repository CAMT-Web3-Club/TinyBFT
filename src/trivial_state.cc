#include "trivial_state.h"

#include <stdio.h>
#include <sys/mman.h>

#include <cstdlib>
#include <cstring>

#include "Fetch.h"
#include "Log.t"
#include "Node.h"
#include "Partition.h"
#include "Replica.h"
#include "libbyz.h"
#include "th_assert.h"

namespace libbyzea {

TrivialState::TrivialState(Replica *replica, char *state, int state_len)
    : replica_(replica),
      fetching_(false),
      last_replier_(0),
      meta_data_cert_(new Meta_data_cert),
      last_fetch_time_(zeroTime()),
      next_block_(0),
      fetch_checkpoint_id_(-1),
      fetch_checkpoint_digest_(),
      fetch_state_(new uint8_t[state_len]),
      checking_(false),
      state_(reinterpret_cast<uint8_t *>(state)),
      state_len_(size_t(state_len)),
      num_blocks_(state_len_ / Block_size),
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

  STOP_CC(rollback_cycles);

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

void TrivialState::start_fetch(Seqno last_exec, Seqno checkpoint,
                               Digest *checkpoint_digest,
                               [[maybe_unused]] bool stable) {
  START_CC(fetch_cycles);
  if (fetching_) {
    fprintf(stderr, "Would start fetch(%lld, %lld)\n", last_exec, checkpoint);
    STOP_CC(fetch_cycles);
    return;
  }
  fprintf(stderr, "Starting Fetch(%lld, %lld)\n", last_exec, checkpoint);

  INCR_OP(num_fetches);
  fetching_ = true;
  next_block_ = 0;
  last_replier_ = lrand48() % replica_->n();

  if (last_checkpoint_id_ >= 0 && last_exec > last_checkpoint_id_) {
    this->checkpoint(last_exec);
  }

  meta_data_cert_->clear();

  fetch_checkpoint_id_ = checkpoint;
  if (checkpoint_digest != nullptr) {
    fetch_checkpoint_digest_ = *checkpoint_digest;
  }

  STOP_CC(fetch_cycles);
  send_fetch(true);
}

void TrivialState::send_fetch(bool change_replier) {
  START_CC(fetch_cycles);

  last_fetch_time_ = currentTime();
  Request_id rid = replica_->new_rid();
  replica_->principal()->set_last_fetch_rid(rid);

  int replier = -1;
  if (fetch_checkpoint_id_ >= 0) {
    if (change_replier) {
      do {
        last_replier_ = lrand48() % replica_->n();
      } while (last_replier_ == replica_->id());
    }
    replier = last_replier_;
  }

  // We do not have partition levels, so we set it to the last partition ,which
  // would be the actual data partitions in the original model.
  // TODO: update last up-to-date sequence number once COW is implemented
  // properly.
  Fetch f(rid, last_checkpoint_id_, PLevels - 1, next_block_,
          fetch_checkpoint_id_, replier);
  // fprintf(stderr, "Sending fetch block = %d, checkpoint = %lld, replier =
  // %d\n",
  //         next_block_, fetch_checkpoint_id_, replier);
  replica_->send(&f, Node::All_replicas);

  if (!meta_data_cert_->has_mine()) {
    auto last_stable = checkpoint_log_.head_seqno();
    if (!checkpoint_log_.fetch(last_stable).is_cleared() &&
        fetch_checkpoint_id_ <= last_checkpoint_id_) {
      auto mdd = new Meta_data_d(rid, PLevels - 1, num_blocks_, last_stable);
      for (auto n = last_stable; n <= last_checkpoint_id_;
           n += checkpoint_interval) {
        auto &digest = checkpoint_log_.fetch(n).digest();
        if (digest.is_zero()) {
          continue;
        }
        fprintf(stderr, "Adding mine for %lld\n", n);
        mdd->add_digest(n, digest);
      }

      meta_data_cert_->add(mdd, true);
    }
  }

  STOP_CC(fetch_cycles);
}

void TrivialState::start_check([[maybe_unused]] Seqno last_exec) {
  fprintf(stderr, "START_CHECK\n");
}

void TrivialState::check_state() { fprintf(stderr, "CHECK_STATE\n"); }

bool TrivialState::shutdown(FILE *output, Seqno last_stable) {
  size_t got_bytes_written = 0;
  size_t want_bytes_written = 0;

  Digest digest(reinterpret_cast<char *>(state_), state_len_);
  got_bytes_written += fwrite(&digest, sizeof(digest), 1, output);
  want_bytes_written += sizeof(digest);
  got_bytes_written += fwrite(state_, state_len_, 1, output);
  want_bytes_written += state_len_;
  got_bytes_written +=
      fwrite(&last_checkpoint_id_, sizeof(last_checkpoint_id_), 1, output);
  want_bytes_written += sizeof(last_checkpoint_id_);

  bool ok = true;
  if (!fetching_) {
    for (auto i = last_stable; i < last_stable + max_out; i++) {
      auto &record = checkpoint_log_.fetch(i);
      if (!record.is_cleared()) {
        continue;
      }

      got_bytes_written += fwrite(&i, sizeof(i), 1, output);
      want_bytes_written += sizeof(i);
      ok &= record.marshal(output);
    }
  }
  Seqno end = -1;
  got_bytes_written += fwrite(&end, sizeof(end), 1, output);
  want_bytes_written += sizeof(end);

  return ok && got_bytes_written == want_bytes_written;
}

bool TrivialState::restart(FILE *input, Replica *replica, Seqno last_stable,
                           Seqno last_executed, bool corrupt) {
  replica_ = replica;

  if (corrupt) {
    checkpoint_log_.clear(last_stable);
    last_checkpoint_id_ = -1;
    return false;
  }

  size_t got_bytes_read = 0;
  size_t want_bytes_read = 0;
  Digest digest;
  got_bytes_read += fread(&digest, sizeof(digest), 1, input);
  want_bytes_read += sizeof(digest);
  got_bytes_read += fread(state_, state_len_, 1, input);
  want_bytes_read += state_len_;
  got_bytes_read +=
      fread(&last_checkpoint_id_, sizeof(last_checkpoint_id_), 1, input);
  want_bytes_read += sizeof(last_checkpoint_id_);

  Seqno checkpoint;
  bool ok = true;
  while (checkpoint >= 0) {
    got_bytes_read += fread(&checkpoint, sizeof(checkpoint), 1, input);
    want_bytes_read += sizeof(last_stable);
    if (checkpoint < last_stable || checkpoint > last_executed) {
      return false;
    }

    auto &record = checkpoint_log_.fetch(checkpoint);
    ok &= record.unmarshal(input, state_len_);
  }

  return ok && got_bytes_read == want_bytes_read;
}

bool TrivialState::enforce_bound(Seqno bound, Seqno known_stable,
                                 bool corrupt) {
  // TODO: Check last modified states of all blocks.
  if (corrupt || checkpoint_log_.head_seqno() >= bound) {
    last_checkpoint_id_ = -1;
    checkpoint_log_.clear(known_stable);
    return false;
  }

  return true;
}

void TrivialState::handle(Meta_data *m) {
  // We should never receive this
  th_fail("Unexpected Meta_data received");
  delete m;
}

void TrivialState::finish_fetch() {
  th_assert(next_block_ >= num_blocks_, "invalid fetch state");

  START_CC(fetch_cycles);
  Digest d(reinterpret_cast<char *>(fetch_state_), state_len_);
  if (d != fetch_checkpoint_digest_) {
    // Fetched state is not correct, so restart fetch and choose another
    // replica.
    next_block_ = 0;
    STOP_CC(fetch_cycles);
    send_fetch(true);
    return;
  }

  th_assert(last_checkpoint_id_ < fetch_checkpoint_id_, "Invalid state");
  if (!checkpoint_log_.within_range(fetch_checkpoint_id_)) {
    checkpoint_log_.truncate(fetch_checkpoint_id_ - max_out);
  }
  auto &record = checkpoint_log_.fetch(fetch_checkpoint_id_);
  record.snapshot(fetch_state_, state_len_);
  last_checkpoint_id_ = fetch_checkpoint_id_;
  std::memcpy(state_, fetch_state_, state_len_);
  // Fetching must be set to false before calling replica_->new_state(),
  // otherwise the replica's logs are not updated correctly.
  fetching_ = false;
  meta_data_cert_->clear();
  replica_->new_state(fetch_checkpoint_id_);
  fetch_checkpoint_id_ = -1;
  fetch_checkpoint_digest_.zero();

  STOP_CC(fetch_cycles);
}

void TrivialState::handle(Meta_data_d *m) {
  INCR_OP(meta_datad_fetched);
  INCR_CNT(meta_datad_bytes, m->size());
  START_CC(fetch_cycles);

  Request_id current_fetch_rid = replica->principal()->last_fetch_rid();
  if (!fetching_ || current_fetch_rid != m->request_id() ||
      m->last_stable() < last_checkpoint_id_ ||
      m->last_stable() < fetch_checkpoint_id_) {
    delete m;
    STOP_CC(fetch_cycles);
    return;
  }

  INCR_OP(meta_datad_fetched_a);
  if (!meta_data_cert_->add(m)) {
    return;
  }

  Seqno chosen_checkpoint;
  Digest chosen_digest;
  if (!meta_data_cert_->cvalue(chosen_checkpoint, chosen_digest)) {
    return;
  }

  // The certificate is complete, we have found a valid checkpoint digest.
  fetch_checkpoint_id_ = chosen_checkpoint;
  fetch_checkpoint_digest_ = chosen_digest;

  meta_data_cert_->clear();

  if (next_block_ >= num_blocks_) {
    fprintf(stderr, "finish_fetch from Meta_data_d\n");
    finish_fetch();
    return;
  }

  STOP_CC(fetch_cycles);
  send_fetch(true);
}

void TrivialState::handle(Data *m) {
  INCR_OP(num_fetched);
  START_CC(fetch_cycles);
  if (!fetching_ || m->index() != next_block_) {
    STOP_CC(fetch_cycles);
    delete m;
    return;
  }

  int i = next_block_ * Block_size;
  std::memcpy(&fetch_state_[i], m->data(), Block_size);
  next_block_++;
  STOP_CC(fetch_cycles);
  if (next_block_ < num_blocks_ || fetch_checkpoint_digest_.is_zero()) {
    send_fetch();
  } else {
    finish_fetch();
  }
  delete m;
}

char *TrivialState::checkpoint_data(Seqno checkpoint, int block) {
  auto &record = checkpoint_log_.fetch(checkpoint);
  th_assert(!record.is_cleared(), "Invalid state");
  int i = block * Block_size;
  char *data = record.fetch();
  return &data[i];
}

bool TrivialState::handle(Fetch *m, Seqno last_stable) {
  if (!m->verify()) {
    delete m;
    return false;
  }

  auto block = m->index();
  Seqno requested_checkpoint = m->checkpoint();
  Principal *sender = replica_->i_to_p(m->id());
  if (sender->last_fetch_rid() >= m->request_id() || block < 0 ||
      block > num_blocks_) {
    delete m;
    return false;
  }

  if (requested_checkpoint >= 0 && block < num_blocks_ &&
      m->replier() == replica_->id()) {
    Seqno chosen_checkpoint = -1;
    if (checkpoint_log_.within_range(requested_checkpoint) &&
        !checkpoint_log_.fetch(requested_checkpoint).is_cleared()) {
      chosen_checkpoint = requested_checkpoint;
    }
    if (chosen_checkpoint >= 0) {
      auto data = checkpoint_data(chosen_checkpoint, block);
      // TODO: actual last modified
      Data d(block, chosen_checkpoint, data);
      replica->send(&d, m->id());
    }
  }

  if (requested_checkpoint < last_stable && m->last_uptodate() <= last_stable &&
      !checkpoint_log_.fetch(last_stable).is_cleared()) {
    Meta_data_d mdd(m->request_id(), PLevels - 1, block, last_stable);
    auto &record = checkpoint_log_.fetch(last_stable);
    Digest d;
    record.digest(d);
    mdd.add_digest(last_stable, d);
    mdd.authenticate(sender);
    replica->send(&mdd, m->id());
    // fprintf(stderr, "Sending meta-data-d i=%d, last_checkpoint=%lld\n",
    //     m->id(), last_checkpoint_id_);
  }

  delete m;
  return true;
}

void TrivialState::mark_stale() { meta_data_cert_->clear(); }

void TrivialState::simulate_reboot() {
  START_CC(reboot_time);
  static constexpr unsigned long reboot_usec = 30000000;
  Cycle_counter inv_time;

  // Invalidate state pages to force reading from disk after reboot
  inv_time.start();
#ifdef NO_STATE_TRANSLATION
  msync(state_, num_blocks_ * Block_size, MS_INVALIDATE);
#endif
  inv_time.stop();

  usleep(reboot_usec - inv_time.elapsed() / clock_mhz);
  STOP_CC(reboot_time);
}
}  // namespace libbyzea
