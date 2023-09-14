#include "hmac.h"

#include <mbedtls/md.h>

namespace libbyzea {

bool Hmac::equal(const char *mac1, const char *mac2, size_t len) {
  uint32_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= mac1[i] ^ mac2[i];
  }

  return bool(((diff ^ 0U) - 1) >> 31);
}

Hmac::Hmac(mbedtls_md_type_t hash_alg) : initialized_(false), ctx_() {
  mbedtls_md_init(&ctx_);
  auto info = mbedtls_md_info_from_type(hash_alg);
  mbedtls_md_setup(&ctx_, info, true);
}

bool Hmac::is_initialized() const { return initialized_; }

Hmac::Hmac(mbedtls_md_type_t hash_alg, const char *key, size_t key_len)
    : Hmac(hash_alg) {
  mbedtls_md_hmac_starts(&ctx_, reinterpret_cast<const unsigned char *>(key),
                         key_len);
  initialized_ = true;
}

Hmac::~Hmac() { mbedtls_md_free(&ctx_); }

void Hmac::replace_key(const char *key, size_t key_len) {
  mbedtls_md_hmac_starts(&ctx_, reinterpret_cast<const unsigned char *>(key),
                         key_len);
  initialized_ = true;
}

void Hmac::update(const char *str, size_t len) {
  mbedtls_md_hmac_update(&ctx_, reinterpret_cast<const unsigned char *>(str),
                         len);
}

void Hmac::sum(char *buf) {
  mbedtls_md_hmac_finish(&ctx_, reinterpret_cast<unsigned char *>(buf));
}

bool Hmac::equals(const char *other_mac) {
  char mac[digest_size()];
  sum(mac);
  return Hmac::equal(mac, other_mac, digest_size());
}

void Hmac::reset() { mbedtls_md_hmac_reset(&ctx_); }

size_t Hmac::digest_size() const {
  return mbedtls_md_get_size(mbedtls_md_info_from_ctx(&ctx_));
}

}  // namespace libbyzea
