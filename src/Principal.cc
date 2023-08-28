#include "Principal.h"

#include <stdlib.h>
#include <strings.h>

#include <cstring>
#include <string>

#include "Node.h"
#include "Reply.h"
#include "rsa_public_key.h"

namespace libbyzea {

Principal::Principal(MEM_STATS_PARAM int i, Addr a,
                     mbedtls_ctr_drbg_context *drbg, char *key_filename) {
  id = i;
  addr = a;
  drbg_ctx = drbg;
  last_fetch = 0;

  if (key_filename != nullptr) {
    std::string filename(key_filename, std::strlen(key_filename));
    pkey = new libbyzea::RsaPublicKey(filename, drbg);
    ssize = pkey->size() + sizeof(size_t) + 1;
  }

  for (int j = 0; j < 4; j++) {
    kin[j] = 0;
    kout[j] = 0;
  }

#ifndef USE_SECRET_SUFFIX_MD5
  ctx_in = 0;
  ctx_out = umac_new((char *)kout);
#endif

  tstamp = 0;
  my_tstamp = zeroTime();
  MEM_STATS_GUARD_POP();
}

Principal::~Principal() { delete pkey; }

void Principal::set_in_key(const unsigned *k) {
  memcpy(kin, k, Key_size);

#ifndef USE_SECRET_SUFFIX_MD5
  if (ctx_in) umac_delete(ctx_in);
  ctx_in = umac_new((char *)kin);
#endif
}

#ifdef USE_SECRET_SUFFIX_MD5
bool Principal::verify_mac(const char *src, unsigned src_len, const char *mac,
                           unsigned *k) {
  // Do not accept MACs sent with uninitialized keys.
  if (k[0] == 0) return false;

  MD5_CTX context;
  unsigned int digest[4];

  MD5Init(&context);
  MD5Update(&context, src, src_len);
  MD5Update(&context, (char *)k, 16);
  MD5Final(digest, &context);
  return !memcmp(digest, mac, MAC_size);
}

void Principal::gen_mac(const char *src, unsigned src_len, char *dst,
                        unsigned *k) {
  MD5_CTX context;
  unsigned int digest[4];

  MD5Init(&context);
  MD5Update(&context, src, src_len);
  MD5Update(&context, (char *)k, 16);
  MD5Final(digest, &context);

  // Copy to destination and truncate output to MAC_size
  memcpy(dst, (char *)digest, MAC_size);
}
#else
bool Principal::verify_mac(const char *src, unsigned src_len, const char *mac,
                           const char *unonce, umac_ctx_t ctx) {
  // Do not accept MACs sent with uninitialized keys.
  if (ctx == 0) return false;

  char tag[20];
  umac(ctx, (char *)src, src_len, tag, (char *)unonce);
  umac_reset(ctx);
  return !memcmp(tag, mac, UMAC_size);
}

long long Principal::umac_nonce = 0;

void Principal::gen_mac(const char *src, unsigned src_len, char *dst,
                        const char *unonce, umac_ctx_t ctx) {
  umac(ctx, (char *)src, src_len, dst, (char *)unonce);
  umac_reset(ctx);
}

#endif

void Principal::set_out_key(unsigned *k, ULong t) {
  if (t > tstamp) {
    memcpy(kout, k, Key_size);

#ifndef USE_SECRET_SUFFIX_MD5
    if (ctx_out) umac_delete(ctx_out);
    ctx_out = umac_new((char *)kout);
#endif

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

  const std::string msg(src, src_len);
  bool ret =
      pkey->verify(msg, reinterpret_cast<const uint8_t *>(sig), signature_len);

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

  std::string plaintext(src, src_len);
  // This is rather inefficient if message is big but messages will
  // be small.
  int err = pkey->encrypt(plaintext, reinterpret_cast<uint8_t *>(dst), dst_len);
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
