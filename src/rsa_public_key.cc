#include "rsa_public_key.h"

#include <mbedtls/pk.h>

#include "th_assert.h"

namespace libbyzea {

RsaPublicKey::RsaPublicKey(const std::string &key_filename,
                           mbedtls_ctr_drbg_context *rng_ctx)
    : rng_ctx_(rng_ctx) {
  mbedtls_pk_context ctx;
  mbedtls_pk_init(&ctx);
  int err = mbedtls_pk_parse_public_keyfile(&ctx, key_filename.c_str());
  if (err) {
    th_fail("error while reading PEM encoded public key");
  }
  if (mbedtls_pk_get_type(&ctx) != MBEDTLS_PK_RSA) {
    th_fail("public key file is not an RSA public key");
  }

  // MbedTLS' public key abstraction layer by default uses PKCSv1.5. We want to
  // use v2.1 instead, so OAEP encryption and PSS signatures are used.
  ctx_ = mbedtls_pk_rsa(ctx);
  mbedtls_rsa_set_padding(ctx_, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
}

RsaPublicKey::~RsaPublicKey() { mbedtls_rsa_free(ctx_); }

int RsaPublicKey::encrypt(const std::string &plaintext, uint8_t *dest,
                          size_t dest_len) {
  if (dest == nullptr || dest_len < size()) {
    return EINVAL;
  }

  int err = mbedtls_rsa_pkcs1_encrypt(
      ctx_, mbedtls_ctr_drbg_random, rng_ctx_, plaintext.length(),
      reinterpret_cast<const unsigned char *>(plaintext.c_str()), dest);

  return err;
}

bool RsaPublicKey::verify(const std::string &msg, const uint8_t *signature,
                          size_t signature_len) {
  if (signature == nullptr || signature_len < size()) {
    printf("%p == nullptr || %lu < %lu?", signature, signature_len, size());
    return false;
  }

  // TODO: use hash instead of the whole raw message
  int err = mbedtls_rsa_pkcs1_verify(
      ctx_, MBEDTLS_MD_NONE, msg.length(),
      reinterpret_cast<const unsigned char *>(msg.c_str()), signature);
  printf("verify: %d\n", err);
  return err == 0;
}

size_t RsaPublicKey::size() const { return mbedtls_rsa_get_len(ctx_); }

}  // namespace libbyzea
