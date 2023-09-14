#ifndef LIBBYZEA_HMAC_H_
#define LIBBYZEA_HMAC_H_

#include <mbedtls/md.h>
#include <stddef.h>

namespace libbyzea {

/**
 * Keyed-Hash Message Authentication Code (HMAC) implementation.
 */
class Hmac {
 public:
  /**
   * @brief Compare two MACs for equality.
   *
   * Compare two MACs for equality without leaking timing information.
   */
  static bool equal(const char *mac1, const char *mac2, size_t len);

  Hmac() = delete;

  /**
   * @brief Initialize an HMAC for thegiven algorithm, without setting a key.
   *
   * Initialize a new HMC using the givne has algorithm, without setting a
   * secret key, yet. The key can be set later using replace_key().
   */
  Hmac(mbedtls_md_type_t hash_alg);

  /**
   * @brief Initialize an HMAC using the given hash algorithm and key.
   *
   * Initialize a new HMAC using the given hash algorithm and secret key.
   */
  Hmac(mbedtls_md_type_t hash_alg, const char *key, size_t key_len);

  virtual ~Hmac();

  /** @brief Return true if this Hmac is ready to be used. */
  bool is_initialized() const;

  /** @brief Replace the current secret key with a new one. */
  void replace_key(const char *key, size_t key_len);

  /** @brief Add more data to the running MAC generation. */
  void update(const char *str, size_t len);

  /**
   * @brief Write the HMACs hash sum to buf.
   *
   * Finish computation of the HMAC and write the resulting digest to a buffer.
   * The buffer has to be digest_size() bytes large.
   */
  void sum(char *buf);

  /**
   * @brief Finish HMAC computation and compare with other HMAC.
   *
   * Finish HMAC computation of this instance and compare the result with
   * other_mac.
   */
  bool equals(const char *other_mac);

  /** @brief Reset the HMAC to its initial state. */
  void reset();

  /** @brief Return the HMAC digest size in bytes. */
  size_t digest_size() const;

 private:
  bool initialized_;
  mbedtls_md_context_t ctx_;
};

}  // namespace libbyzea
#endif  // !LIBBYZEA_HMAC_H_
