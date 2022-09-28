#ifndef LIBBYZ_RSA_PRIVATE_KEY_H_
#define LIBBYZ_RSA_PUBLIC_KEY_H_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/rsa.h>

#include <string>

namespace libbyzea {

class RsaPrivateKey {
 public:
  /**
   * @brief Initialize an RSA private key.
   *
   * Initialize an RSA private key by parsing the given PEM file.
   *
   * @param key_filename the path to the file holding the PEM encoded key.
   * @param rng_ctx context supplied to mbedTLS' random number generator.
   */
  explicit RsaPrivateKey(const std::string &key_filename,
                         mbedtls_ctr_drbg_context *rng_ctx);

  ~RsaPrivateKey();

  /**
   * @brief Decrypt a message.
   *
   * Decrypt a message that has been encrypted with this private key's
   * corresponding public key.
   *
   * @return int 0 on success, an error code otherwise.
   */
  int decrypt(const uint8_t *ciphertext, size_t len, char *dest,
              size_t dest_len, size_t *plaintext_len);

  /**
   * @brief Sign a message.
   *
   * Sign a message that can be verified using the corresponding public key.
   *
   * @return int 0 on sucess, a negative error code otherwise.
   */
  int sign(const std::string &msg, uint8_t *signature, size_t len);

  /**
   * @brief Return the key size in bytes.
   *
   * Return the private key's key size in bytes.
   *
   * @return size_t the key's modulus size in bytes.
   */
  size_t size() const;

 private:
  mbedtls_rsa_context *ctx_;
  mbedtls_ctr_drbg_context *rng_ctx_;
};
}  // namespace libbyzea

#endif  // LIBBYZ_RSA_PRIVATE_KEY_H_
