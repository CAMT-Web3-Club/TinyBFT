#include "rsa_public_key.h"

#include <mbedtls/pk.h>

#include "Digest.h"
#include "mbedtls/md.h"
#include "mem_statistics.h"
#include "th_assert.h"

namespace libbyzea {

RsaPublicKey::RsaPublicKey(const std::string &key_filename,
                           mbedtls_ctr_drbg_context *rng_ctx)
    : rng_ctx_(rng_ctx) {
  MEMSTATS_CALL_STACK_PUSH(RsaPublicKey::RsaPublicKey);
  mbedtls_pk_context ctx;
  mbedtls_pk_init(&ctx);
  MEMSTATS_CALL_STACK_PUSH(mbedtls_pk_parse_public_keyfile);
  int err = mbedtls_pk_parse_public_keyfile(&ctx, key_filename.c_str());
  MEMSTATS_CALL_STACK_POP();
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
  MEMSTATS_CALL_STACK_POP();
}

RsaPublicKey::~RsaPublicKey() { mbedtls_rsa_free(ctx_); }

int RsaPublicKey::encrypt(const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *dest, size_t dest_len) {
  if (dest == nullptr || dest_len < size()) {
    return EINVAL;
  }

  int err = mbedtls_rsa_pkcs1_encrypt(ctx_, mbedtls_ctr_drbg_random, rng_ctx_,
                                      plaintext_len, plaintext, dest);

  return err;
}

bool RsaPublicKey::verify(const uint8_t *msg, size_t msg_len,
                          const uint8_t *signature, size_t signature_len) {
  if (signature == nullptr || signature_len < size()) {
    printf("%p == nullptr || %zu < %zu?", signature, signature_len, size());
    return false;
  }

  // TODO: use hash instead of the whole raw message
  Digest d(reinterpret_cast<const char *>(msg), msg_len);
  int err = mbedtls_rsa_pkcs1_verify(
      ctx_, Digest::ALGORITHM, Digest::SIZE,
      reinterpret_cast<const unsigned char *>(d.digest()), signature);
  return err == 0;
}

size_t RsaPublicKey::size() const { return mbedtls_rsa_get_len(ctx_); }

}  // namespace libbyzea
