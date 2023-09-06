#include "types.h"
#ifndef _Digest_h
#define _Digest_h 1

#include <stddef.h>
#include <stdint.h>

namespace libbyzea {

class Digest {
  static constexpr size_t SIZE = 32;  ///< Size of a Digest in bytes.

 public:
  inline Digest() { zero(); }

  /** @brief Creates a digest for string "s" with length "len" */
  Digest(char *s, size_t len);

  /** @brief Create a digest for a state block. */
  Digest(const char *data, size_t len, int i, Seqno last_modified);

  inline Digest(Digest const &x) {
    for (size_t i = 0; i < num_words(); i++) {
      d[i] = x.d[i];
    }
  }

  inline ~Digest() {}
  // Effects: Deallocates all storage associated with digest.

  inline void zero() {
    for (size_t i = 0; i < num_words(); i++) {
      d[i] = 0;
    }
  }

  inline bool is_zero() const { return d[0] == 0; }

  inline bool operator==(Digest const &x) const {
    uint32_t diff = 0;
    for (size_t i = 0; i < num_words(); i++) {
      diff |= d[i] ^ x.d[i];
    }

    return bool((uint64_t(diff ^ 0U) - 1) >> 63);
  }

  inline bool operator==(unsigned *e) const {
    uint32_t diff = 0;
    for (size_t i = 0; i < num_words(); i++) {
      diff |= d[i] ^ e[i];
    }

    return bool((uint64_t(diff ^ 0U) - 1) >> 63);
  }

  inline bool operator!=(Digest const &x) const { return !(*this == x); }

  inline Digest &operator=(Digest const &x) {
    for (size_t i = 0; i < num_words(); i++) {
      d[i] = x.d[i];
    }
    return *this;
  }

  /**
   * @brief Set digest value to the digest of msg.
   *
   * Compute the digest for msg and set this digest's value to the computed
   * value.
   */
  void set(const char *msg, size_t len);

  /**
   * @brief Set digest value to digest of a state block.
   *
   * Compute the digest for a state block and set this digest's value
   * accordingly.
   */
  void set_state_block(const char *data, size_t len, int i,
                       Seqno last_modified);

  inline int hash() const { return d[0]; }

  char *digest() { return (char *)d; }

  unsigned *udigest() { return d; }

  void print();
  // Effects: Prints digest in stdout.

  /** @brief Return the number of words in a digest. */
  constexpr size_t num_words() const { return SIZE / sizeof(d[0]); }

 private:
  unsigned d[SIZE / sizeof(unsigned)];
};

}  // namespace libbyzea

#endif  // _Digest_h
