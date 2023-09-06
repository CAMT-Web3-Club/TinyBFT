#ifndef LIBBYZEA_DIGEST_BUILDER_H_

#include <mbedtls/sha256.h>

#include "Digest.h"

namespace libbyzea {

/**
 * @brief Utility class for building digests in multiple steps.
 */
class DigestBuilder {
 public:
  DigestBuilder() {
    mbedtls_sha256_init(&ctx_);
    mbedtls_sha256_starts(&ctx_, false);
  };

  ~DigestBuilder(){};

  /**
   * @brief Reset digest computation, starting a new digest computation.
   */
  void reset() { mbedtls_sha256_starts(&ctx_, false); }

  /**
   * @brief Feed data to an ongoing digest computation.
   */
  void update(const char *data, size_t len) {
    mbedtls_sha256_update(&ctx_, reinterpret_cast<const unsigned char *>(data),
                          len);
  }

  /**
   * @brief Finish digest documentation and return output.
   */
  Digest finish() {
    Digest d;
    finish(d);
    return d;
  }

  /**
   * @brief Finish digest documentation and write result to digest.
   */
  void finish(Digest &digest) {
    mbedtls_sha256_finish(&ctx_,
                          reinterpret_cast<unsigned char *>(digest.udigest()));
  };

 private:
  mbedtls_sha256_context ctx_;
};

}  // namespace libbyzea
#endif  // LIBBYZEA_DIGEST_BUILDER_H_
