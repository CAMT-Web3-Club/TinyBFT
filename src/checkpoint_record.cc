#include "checkpoint_record.h"

#include <cstring>

#include "th_assert.h"

namespace libbyzea {

size_t CheckpointRecord::memory_consumption() {
  return sizeof(CheckpointRecord);
}

CheckpointRecord::CheckpointRecord()
    : partitions_(nullptr),
      blocks_(nullptr),
      num_partitions_(),
      num_entries_(0) {}

CheckpointRecord::CheckpointRecord(MEM_STATS_PARAM int num_state_blocks)
    : partitions_(new Part[num_meta_partitions_for_blocks(num_state_blocks)]),
      blocks_(new BlockCopy[num_state_blocks]),
      num_partitions_(),
      num_entries_(0) {
  init_num_partitions(num_state_blocks);

  int partitions = num_meta_partitions_for_blocks(num_state_blocks);
  for (int i = 0; i < partitions; i++) {
    partitions_[i].lm = -1;
  }

  for (int i = 0; i < num_state_blocks; i++) {
    blocks_[i].lm = -1;
  }
  MEM_STATS_GUARD_POP();
}

CheckpointRecord::~CheckpointRecord() {
  delete partitions_;
  delete blocks_;
}

void CheckpointRecord::init(MEM_STATS_PARAM int num_state_blocks) {
  if (partitions_ != nullptr) {
    delete partitions_;
  }
  if (blocks_ != nullptr) {
    delete blocks_;
  }

  init_num_partitions(num_state_blocks);

  partitions_ = new Part[num_meta_partitions_for_blocks(num_state_blocks)];
  blocks_ = new BlockCopy[num_state_blocks];
  num_entries_ = 1;
  clear();
  MEM_STATS_GUARD_POP();
}

void CheckpointRecord::clear() {
  if (num_entries_ == 0) {
    return;
  }

  int partition = 0;
  for (int level = 0; level < PLevels - 1; level++) {
    for (int i = 0; i < num_partitions_[level]; i++) {
      partitions_[partition].lm = -1;
      partition++;
    }
  }
  for (int i = 0; i < num_partitions_[PLevels - 1]; i++) {
    blocks_[i].lm = -1;
  }

  num_entries_ = 0;
  sd.zero();
}

void CheckpointRecord::append(int l, int i, Part &p) {
  auto &partition = find_partition(l, i);
  th_assert(partition.lm == -1, "Invalid state");

  partition.lm = p.lm;
  partition.d = p.d;
  num_entries_++;
}

void CheckpointRecord::append(int i, Seqno lm, Digest &digest, Block &block) {
  th_assert(blocks_[i].lm == -1, "Invalid state");

  blocks_[i].lm = lm;
  blocks_[i].d = digest;
  blocks_[i].data = block;
  num_entries_++;
}

void CheckpointRecord::appendr(int l, int i, Part &p) {
  auto &partition = find_partition(l, i);
  if (partition.lm != -1) {
    return;
  }

  partition.lm = p.lm;
  partition.d = p.d;
  num_entries_++;
}

void CheckpointRecord::appendr(int i, Seqno lm, Digest &digest, Block &block) {
  if (blocks_[i].lm != -1) {
    return;
  }

  blocks_[i].lm = lm;
  blocks_[i].d = digest;
  blocks_[i].data = block;
  num_entries_++;
}

Part *CheckpointRecord::fetch(int l, int i) {
  if (l == (PLevels - 1)) {
    if (blocks_[i].lm == -1) {
      return nullptr;
    }
    return (Part *)&blocks_[i];
  }

  auto &part = find_partition(l, i);
  if (part.lm == -1) {
    return nullptr;
  }

  return &part;
}

void CheckpointRecord::init_num_partitions(int num_blocks) {
  num_partitions_[PLevels - 1] = num_blocks;
  int num_partitions = num_blocks;
  for (int i = PLevels - 2; i >= 0; i--) {
    num_partitions = (num_partitions + PChildren - 1) / PChildren;
    num_partitions_[i] = num_partitions;
  }
}

Part &CheckpointRecord::find_partition(int level, int index) {
  int partition = 0;
  for (int i = 0; i < level; i++) {
    partition += num_partitions_[i];
  }
  partition += index;
  return partitions_[partition];
}

}  // namespace libbyzea
