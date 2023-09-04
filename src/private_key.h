#ifndef LIBBYZEA_PRIVATE_KEY_H_
#define LIBBYZEA_PRIVATE_KEY_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace libbyzea {

class PrivateKey {
 public:
  virtual ~PrivateKey(){};

  /**
   * @brief Decrypt a message.
   *
   * Decrypt a message that has been encrypted with this private key's
   * corresponding public key.
   *
   * @return int 0 on success, an error code otherwise.
   */
  virtual int decrypt(const uint8_t *ciphertext, size_t len, char *dest,
                      size_t dest_len, size_t *plaintext_len) = 0;

  /**
   * @brief Sign a message.
   *
   * Sign a message that can be verified with this key's corresponding public
   * key.
   *
   * @return int 0 on success, an error code otherwise.
   */
  virtual int sign(const uint8_t *msg, size_t msg_len, uint8_t *signature,
                   size_t len) = 0;

  /**
   * @brief Return key size in bytes.
   */
  virtual size_t size() const = 0;
};
}  // namespace libbyzea
#endif  // LIBBYZEA_PRIVATE_KEY_H_
