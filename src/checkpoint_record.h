#ifndef _LIBBYZEA_CHECKPOINT_RECORD_H_
#define _LIBBYZEA_CHECKPOINT_RECORD_H_

#include <stdio.h>
#include <stdlib.h>

#include <cstdint>

#include "Digest.h"

namespace libbyzea {
class CheckpointRecord {
 public:
  CheckpointRecord();
  CheckpointRecord(uint8_t *data, size_t len);

  CheckpointRecord(const CheckpointRecord &) = delete;
  CheckpointRecord(CheckpointRecord &&) = delete;

  CheckpointRecord &operator=(const CheckpointRecord &) = delete;
  CheckpointRecord &operator=(CheckpointRecord &&) = delete;

  ~CheckpointRecord();

  void digest(Digest &digest) const;

  Digest &digest();

  /// @brief Set this checkpoint to the state of data.
  void snapshot(const uint8_t *data, size_t len);

  /**
   * @brief Write the checkpoint's state to a file.
   *
   * Marshal this checkpoint's state and write it to the given file.
   *
   * @return true if the checkpoint has been written to the file, false
   * otherwise.
   */
  bool marshal(FILE *file);

  /** @brief Unmarshal a checkpoint's state from a file.
   *
   * Unmarshal and store the data from file as the state of this checkpoint
   * record.
   *
   * @return true if the checkpoint has been loaded successfully, false
   * otherwise.
   */
  bool unmarshal(FILE *file, size_t len);

  /// @brief Copy this checkpoint to the data.
  void copy(uint8_t *data, size_t len);

  /// @brief return a pointer to the start of the checkpoint's data.
  char *fetch();

  bool is_cleared() const;
  void clear();

  void print();

 private:
  uint8_t *data_;
  size_t len_;
  Digest digest_;
};

inline Digest &CheckpointRecord::digest() { return digest_; }

}  // namespace libbyzea
#endif  // !_LIBBYZEA_CHECKPOINT_RECORD_H_
