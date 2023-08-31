#ifndef _LIBBYZEA_CHECKPOINT_RECORD_H_
#define _LIBBYZEA_CHECKPOINT_RECORD_H_

#include <stdio.h>
#include <stdlib.h>

#include <cstdint>

#include "Digest.h"
#include "Part.h"
#include "Partition.h"
#include "mem_statistics_guard.h"

namespace libbyzea {

class CheckpointRecord {
 public:
  static size_t memory_consumption();

  CheckpointRecord();

  CheckpointRecord(MEM_STATS_PARAM int num_state_blocks);
  // Effects: Creates an empty checkpoint record.

  ~CheckpointRecord();
  // Effects: Deletes record an all parts it contains

  void init(MEM_STATS_PARAM int num_state_blocks);
  // Effects: (Re-)initializes a checkpoint record to a certain number of
  // blocks.

  void clear();
  // Effects: Deletes all parts in record and removes them.

  bool is_cleared() const;
  // Effects: Returns true iff Checkpoint record is not in use.

  void append(int l, int i, Part &p);
  // Requires: fetch(l, i) == 0
  // Effects: Appends partition index "i" at level "l" with value "p"
  // to the record.

  void append(int i, Seqno lm, Digest &digest, Block &block);

  void appendr(int l, int i, Part &p);
  // Effects: Like append but without the requires clause. If fetch(l,
  // i) != 0 it retains the old mapping.

  void appendr(int i, Seqno lm, Digest &digest, Block &block);

  Part *fetch(int l, int i);
  // Effects: If there is a partition with index "i" from level "l" in
  // this, returns a pointer to its information. Otherwise, returns 0.

  Part &partition(int l, int i);

  BlockCopy &block(int i);

  int num_entries() const;
  // Effects: Returns the number of entries in the record.

  class Iter {
   public:
    inline Iter(CheckpointRecord *r) : level_(0), index_(0), record_(r) {}
    // Effects: Return an iterator for the partitions in r.

    inline bool get(int &level, int &index, Part *&p) {
      // Effects: Modifies "level", "index" and "p" to contain
      // information for the next partition in "r" and returns
      // true. Unless there are no more partitions in "r" in which case
      // it returns false.
      find_next();
      if (level_ >= PLevels) {
        return false;
      }

      if (level_ < PLevels - 1) {
        auto &part = record_->find_partition(level_, index_);
        p = &part;
      } else {
        p = (Part *)&record_->blocks_[index_];
      }
      level = level_;
      index = index_;
      index_++;

      return true;
    }

   private:
    inline void find_next() {
      if (level_ >= PLevels) {
        return;
      }

      for (int l = level_; l < PLevels - 1; l++) {
        for (int i = index_; i < PSize[l]; i++) {
          auto &part = record_->find_partition(l, i);
          if (part.lm >= 0) {
            level_ = l;
            index_ = i;
            return;
          }
        }

        index_ = 0;
        level_ = l + 1;
      }

      for (int i = index_; i < PSize[level_]; i++) {
        if (record_->blocks_[i].lm >= 0) {
          index_ = i;
          return;
        }
      }
      level_++;
    }

    int level_;
    int index_;
    CheckpointRecord *record_;
  };
  friend class Iter;

  void print();
  // Effects: Prints description of this to stdout

  Digest sd;  // State digest at the time the checkpoint was taken.

 private:
  Part &find_partition(int level, int index);

  Part *partitions_;
  BlockCopy *blocks_;
  int num_entries_;
};

inline int CheckpointRecord::num_entries() const { return num_entries_; }

inline bool CheckpointRecord::is_cleared() const { return sd.is_zero(); }

inline Part &CheckpointRecord::partition(int level, int index) {
  return find_partition(level, index);
}

inline BlockCopy &CheckpointRecord::block(int index) { return blocks_[index]; }

}  // namespace libbyzea
#endif  // !_LIBBYZEA_CHECKPOINT_RECORD_H_
