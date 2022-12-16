#include "checkpoint_record.h"

#include <cstring>

#include "th_assert.h"

namespace libbyzea {
CheckpointRecord::CheckpointRecord() : data_(nullptr), len_(0), digest_() {}

CheckpointRecord::CheckpointRecord(uint8_t *data, size_t len)
    : data_(new uint8_t[len]),
      len_(len),
      digest_(reinterpret_cast<char *>(data), unsigned(len)) {
  std::memcpy(this->data_, data, len);
}

CheckpointRecord::~CheckpointRecord() { delete data_; }

void CheckpointRecord::digest(Digest &digest) const { digest = digest_; }

void CheckpointRecord::snapshot(const uint8_t *data, size_t len) {
  th_assert(data_ == nullptr, "checkpoint is already in use");

  data_ = new uint8_t[len];
  len_ = len;
  std::memcpy(data_, data, len_);
  digest_ = Digest((char *)(data_), int(len_));
}

bool CheckpointRecord::marshal(FILE *file) {
  size_t got_bytes_written;
  size_t want_bytes_written;

  got_bytes_written = fwrite(&digest_, sizeof(digest_), 1, file);
  want_bytes_written = sizeof(digest_);
  got_bytes_written += fwrite(data_, len_, 1, file);
  want_bytes_written += len_;

  return got_bytes_written == want_bytes_written;
}

bool CheckpointRecord::unmarshal(FILE *file, size_t len) {
  size_t got_bytes_read;
  size_t want_bytes_read;

  clear();
  data_ = new uint8_t[len];
  len_ = len;

  Digest digest;
  got_bytes_read = fread(&digest, sizeof(digest), 1, file);
  want_bytes_read = sizeof(digest);
  got_bytes_read += fread(data_, len_, 1, file);
  want_bytes_read += len;
  digest_ = Digest((char *)(data_), int(len_));

  return got_bytes_read == want_bytes_read && digest == digest_;
}

void CheckpointRecord::copy(uint8_t *data, size_t len) {
  th_assert(data != nullptr && data_ != nullptr,
            "checkpoint or receiving data pointer is null");
  th_assert(len >= len_, "buffer is too small to receive checkpoint state");

  std::memcpy(data, data_, len_);
}

char *CheckpointRecord::fetch() { return reinterpret_cast<char *>(data_); }

bool CheckpointRecord::is_cleared() const { return (data_ == nullptr); }

void CheckpointRecord::clear() {
  if (data_ == nullptr) {
    return;
  }

  delete data_;
  data_ = nullptr;
  len_ = 0;
}

void CheckpointRecord::print() {
  printf("Checkpoint record: %lu bytes\n", len_);
  printf("    Digest: %s\n", digest_.digest());
}
}  // namespace libbyzea
