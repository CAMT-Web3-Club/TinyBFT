#ifndef LIBBYZ_RSA_PUBLIC_KEY_H_
#define LIBBYZ_RSA_PUBLIC_KEY_H_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/rsa.h>

#include <string>

#include "public_key.h"

namespace libbyzea {

class RsaPublicKey : public PublicKey {
 public:
  /**
   * @brief Initialize a new RSA public key.
   *
   * @param key_filename the path to the file holding the PEM encoded key.
   * @param rng_ctx context supplied to mbedTLS' random number generator.
   */
  explicit RsaPublicKey(const std::string &key_filename,
                        struct mbedtls_ctr_drbg_context *rng_ctx);

  RsaPublicKey(const RsaPublicKey &) = delete;
  RsaPublicKey &operator=(const RsaPublicKey &) = delete;

  RsaPublicKey(RsaPublicKey &&) = default;
  RsaPublicKey &operator=(RsaPublicKey &&) = default;

  ~RsaPublicKey();

  /**
   * @brief Encrypt a message using RSA-OAEP.
   *
   * Encrypt a plaintext using the RSA-OAEP encryption scheme.
   *
   * @param plaintext the message to encrypt.
   * @param ciphertext the ciphertext's destination buffer. The buffer must be
   * at least {@sa size()} bytes large.
   * @param ciphertext_len the length of the ciphertext buffer.
   * @return int 0 on success, an error code otherwise.
   */
  int encrypt(const std::string &plaintext, uint8_t *ciphertext,
              size_t ciphertext_len) override;

  /**
   * @brief Verify the signature of a message.
   *
   * Verify that the given signature is a valid RSA-PSS signature for msg.
   *
   * @param msg the message.
   * @param signature the signature of the given message.
   * @param signature_len length of the signature in bytes.
   * @return true if the signature is valid, false otherwise.
   */
  bool verify(const std::string &msg, const uint8_t *signature,
              size_t signature_len) override;

  /**
   * @brief Return the key size in bytes.
   *
   * Return the private key's key size in bytes. This also corresponds to the
   * size of signatures and encrypted messages.
   *
   * @return size_t the key's modulus size in bytes.
   */
  size_t size() const override;

 private:
  mbedtls_rsa_context *ctx_;
  mbedtls_ctr_drbg_context *rng_ctx_;
};
}  // namespace libbyzea

#endif  // LIBBYZ_RSA_PUBLIC_KEY_H_
