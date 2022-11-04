#ifndef _LIBBYZEA_CHECKPOINT_RECORD_H_
#define _LIBBYZEA_CHECKPOINT_RECORD_H_

#include <stdlib.h>

#include "Digest.h"

namespace libbyzea {
class CheckpointRecord {
 public:
  CheckpointRecord();
  CheckpointRecord(uint8_t* data, size_t len);

  CheckpointRecord(const CheckpointRecord&) = delete;
  CheckpointRecord(CheckpointRecord&&) = delete;

  CheckpointRecord& operator=(const CheckpointRecord&) = delete;
  CheckpointRecord& operator=(CheckpointRecord&&) = delete;

  ~CheckpointRecord();

  void digest(Digest& digest) const;

  void snapshot(const uint8_t* data, size_t len);

  /// @brief Copy this checkpoint to the data.
  void copy(uint8_t* data, size_t len);

  bool is_cleared() const;
  void clear();

  void print();

 private:
  uint8_t* data_;
  size_t len_;
  Digest digest_;
};
}  // namespace libbyzea
#endif  // !_LIBBYZEA_CHECKPOINT_RECORD_H_
