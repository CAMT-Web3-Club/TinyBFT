#ifndef LIBBYZEA_PUBLIC_KEY_H_
#define LIBBYZEA_PUBLIC_KEY_H_
#include <cstddef>
#include <cstdint>
#include <string>

namespace libbyzea {

class PublicKey {
 public:
  virtual ~PublicKey(){};

  /**
   * @brief Encrypt a message.
   *
   * Encrypt a plaintext using the key's underlying encryption scheme. The
   * message can be decrypted with this key's corresponding private key.
   *
   * @param plaintext the message to encrypt.
   * @param ciphertext the ciphertext's destination buffer. The buffer must be
   *                   at least {@sa size()} bytes large.
   * @param ciphertext_len the length of the ciphertext buffer.
   * @return int 0 on success, an error code otherwise.
   */
  virtual int encrypt(const std::string &plaintext, uint8_t *ciphertext,
                      size_t ciphertext_len) = 0;

  /**
   * @brief Verify the signature of a message.
   *
   * Verify that signature is a valid signature for msg that has been created
   * with the public key's corresponding private key.
   *
   * @param msg the message.
   * @param signature the signature of the given message.
   * @param signature_len length of the signature in bytes.
   * @return true if the signature is valid, false otherwise.
   */
  virtual bool verify(const std::string &msg, const uint8_t *signature,
                      size_t signature_len) = 0;

  /**
   * @brief Return the key size in bytes.
   *
   * Return the key's size in bytes. Typically, this also corresponds to the
   * size of signatures.
   *
   * @return size_t the key's size in bytes.
   */
  virtual size_t size() const = 0;
};
}  // namespace libbyzea
#endif  // LIBBYZEA_PUBLIC_KEY_H_
