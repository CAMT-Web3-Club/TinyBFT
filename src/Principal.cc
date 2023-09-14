#include "Principal.h"

#include <stdlib.h>
#include <strings.h>

#include <cstring>
#include <string>

#include "Node.h"
#include "Reply.h"
#include "hmac.h"
#include "mbedtls/md.h"
#include "rsa_public_key.h"
#include "umac.h"

namespace libbyzea {

Principal::Principal(MEM_STATS_PARAM int i, Addr a,
                     mbedtls_ctr_drbg_context *drbg, char *key_filename)
    : id(i),
      addr(a),
      drbg_ctx(drbg),
      last_fetch(0),
      hmac_in(MBEDTLS_MD_SHA256),
      hmac_out(MBEDTLS_MD_SHA256) {
  if (key_filename != nullptr) {
    std::string filename(key_filename, std::strlen(key_filename));
    pkey = new libbyzea::RsaPublicKey(filename, drbg);
    ssize = ALIGNED_SIZE(pkey->size() + sizeof(uint32_t));
  }

  for (int j = 0; j < 4; j++) {
    kin[j] = 0;
    kout[j] = 0;
  }
  hmac_out.replace_key(reinterpret_cast<const char *>(kout), Key_size);

  tstamp = 0;
  my_tstamp = zeroTime();
  MEM_STATS_GUARD_POP();
}

Principal::~Principal() { delete pkey; }

void Principal::set_in_key(const unsigned *k) {
  memcpy(kin, k, Key_size);
  hmac_in.replace_key(reinterpret_cast<const char *>(kin), Key_size);
}

bool Principal::verify_mac(const char *src, unsigned src_len, const char *mac,
                           const char *unonce, Hmac &hmac) {
  // Do not accept MACs sent with uninitialized keys.
  if (!hmac.is_initialized()) {
    return false;
  }

  hmac.reset();
  hmac.update(unonce, UNonce_size);
  hmac.update(src, src_len);
  return hmac.equals(mac);
}

long long Principal::umac_nonce = 0;

void Principal::gen_mac(const char *src, unsigned src_len, char *dst,
                        const char *unonce, Hmac &hmac) {
  hmac.reset();
  hmac.update(unonce, UNonce_size);
  hmac.update(src, src_len);
  hmac.sum(dst);
}

void Principal::set_out_key(unsigned *k, ULong t) {
  if (t > tstamp) {
    memcpy(kout, k, Key_size);
    hmac_out.replace_key(reinterpret_cast<const char *>(k), Key_size);
    tstamp = t;
    my_tstamp = currentTime();
  }
}

bool Principal::verify_signature(const char *src, unsigned src_len,
                                 const char *sig, bool allow_self) {
  // Principal never verifies its own authenticator.
  if ((id == node->id()) && !allow_self) return false;
  INCR_OP(num_sig_ver);
  START_CC(sig_ver_cycles);

  uint32_t signature_len;
  memcpy(&signature_len, sig, sizeof(signature_len));
  sig += sizeof(signature_len);
  if (signature_len + sizeof(signature_len) > sig_size()) {
    printf("%zx > %zx?\n", (signature_len + sizeof(signature_len)), sig_size());
    STOP_CC(sig_ver_cycles);
    return false;
  }

  bool ret =
      pkey->verify(reinterpret_cast<const uint8_t *>(src), src_len,
                   reinterpret_cast<const uint8_t *>(sig), signature_len);

  STOP_CC(sig_ver_cycles);
  return ret;
}

unsigned Principal::encrypt(const char *src, uint32_t src_len, char *dst,
                            unsigned dst_len) {
  uint32_t ciphertext_len = pkey->size();
  unsigned total_len =
      ciphertext_len + sizeof(src_len) + sizeof(ciphertext_len);
  if (dst_len < total_len) {
    return 0;
  }

  memcpy(dst, (char *)&src_len, sizeof(src_len));
  dst += sizeof(src_len);
  memcpy(dst, (char *)&ciphertext_len, sizeof(ciphertext_len));
  dst += sizeof(ciphertext_len);
  dst_len -= (sizeof(src_len) + sizeof(ciphertext_len));

  // This is rather inefficient if message is big but messages will
  // be small.
  int err = pkey->encrypt(reinterpret_cast<const uint8_t *>(src), src_len,
                          reinterpret_cast<uint8_t *>(dst), dst_len);
  th_assert(err == 0, "unexpected error while encrypting message");

  return total_len;
}

void random_nonce(unsigned *n) {
  int err = mbedtls_ctr_drbg_random(node->drbg_context(),
                                    reinterpret_cast<uint8_t *>(n), Nonce_size);
  th_assert(err == 0, "failed to generate random nonce");
}

int random_int() {
  int i;
  int err = mbedtls_ctr_drbg_random(node->drbg_context(),
                                    reinterpret_cast<uint8_t *>(&i), sizeof(i));
  th_assert(err == 0, "failed to generate random integer");
  return i;
}

}  // namespace libbyzea
