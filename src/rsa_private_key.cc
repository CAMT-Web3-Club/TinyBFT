#include "rsa_private_key.h"

#include <mbedtls/pk.h>

#include <cstdio>

#include "Digest.h"
#include "mbedtls/md.h"
#include "mem_statistics.h"
#include "th_assert.h"

namespace libbyzea {

RsaPrivateKey::RsaPrivateKey(const std::string &key_filename,
                             mbedtls_ctr_drbg_context *rng_ctx)
    : rng_ctx_(rng_ctx) {
  MEMSTATS_CALL_STACK_PUSH(RsaPrivateKey::RsaPrivateKey);
  mbedtls_pk_init(&pk_ctx_);

  // TODO: we might want to encrypt these.
  MEMSTATS_CALL_STACK_PUSH(mbedtls_pk_parse_keyfile);
  int err = mbedtls_pk_parse_keyfile(&pk_ctx_, key_filename.c_str(), nullptr,
                                     mbedtls_ctr_drbg_random, rng_ctx);
  MEMSTATS_CALL_STACK_POP();
  if (err) {
    th_fail("error while reading PEM encoded public key");
  }
  if (mbedtls_pk_get_type(&pk_ctx_) != MBEDTLS_PK_RSA) {
    th_fail("public key file is not an RSA public key");
  }

  // MbedTLS' public key abstraction layer by default uses PKCSv1.5. We want to
  // use v2.1 instead, so OAEP encryption and PSS signatures are used.
  ctx_ = mbedtls_pk_rsa(pk_ctx_);
  mbedtls_rsa_set_padding(ctx_, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
  MEMSTATS_CALL_STACK_POP();
}

RsaPrivateKey::~RsaPrivateKey() { mbedtls_pk_free(&pk_ctx_); }

int RsaPrivateKey::decrypt(const uint8_t *ciphertext, size_t len, char *dest,
                           size_t dest_len, size_t *plaintext_len) {
  if (dest == nullptr || len != size()) {
    return EINVAL;
  }

  return mbedtls_rsa_pkcs1_decrypt(
      ctx_, mbedtls_ctr_drbg_random, rng_ctx_, plaintext_len, ciphertext,
      reinterpret_cast<unsigned char *>(dest), dest_len);
}

int RsaPrivateKey::sign(const uint8_t *msg, size_t msg_len, uint8_t *signature,
                        size_t len) {
  if (signature == nullptr || len < size()) {
    return EINVAL;
  }

  Digest d(reinterpret_cast<const char *>(msg), msg_len);
  return mbedtls_rsa_pkcs1_sign(
      ctx_, mbedtls_ctr_drbg_random, rng_ctx_, Digest::ALGORITHM, Digest::SIZE,
      reinterpret_cast<const unsigned char *>(d.digest()), signature);
}

size_t RsaPrivateKey::size() const { return mbedtls_rsa_get_len(ctx_); }

}  // namespace libbyzea
