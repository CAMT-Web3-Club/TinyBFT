
#include "Digest.h"

#include <mbedtls/sha256.h>
#include <stdio.h>
#include <string.h>

#include "Statistics.h"

namespace libbyzea {

Digest::Digest(const char *s, size_t len) {
#ifndef NODIGESTS
  INCR_OP(num_digests);
  START_CC(digest_cycles);
  set(s, len);
  STOP_CC(digest_cycles);
#else
  for (int i = 0; i < num_words(); i++) d[i] = 3;
#endif  // NODIGESTS
}

Digest::Digest(const char *data, size_t len, int i, Seqno last_modified) {
#ifndef NODIGESTS
  INCR_OP(num_digests);
  START_CC(digest_cycles);
  set_state_block(data, len, i, last_modified);
  STOP_CC(digest_cycles);
#else
  for (int i = 0; i < num_words(); i++) d[i] = 3;
#endif  // NODIGESTS
}

void Digest::set(const char *s, size_t len) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, false);
  mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char *>(s), len);
  mbedtls_sha256_finish(&ctx, reinterpret_cast<unsigned char *>(&d[0]));
}

void Digest::set_state_block(const char *data, size_t len, int i,
                             Seqno last_modified) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, false);
  mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char *>(&i),
                        sizeof(i));
  mbedtls_sha256_update(&ctx,
                        reinterpret_cast<const unsigned char *>(&last_modified),
                        sizeof(last_modified));
  mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char *>(data),
                        len);
  mbedtls_sha256_finish(&ctx, reinterpret_cast<unsigned char *>(&d[0]));
}

void Digest::print() {
  char buf[SIZE * 2 + 1];
  int n = 0;
  for (size_t i = 0; i < num_words(); i++) {
    int diff = snprintf(&buf[n], sizeof(buf) - n, "%x", d[i]);
    th_assert(diff > 0, "Unexpected snprintf failure.");
    n += diff - 1;
  }
  buf[sizeof(buf) - 1] = '\0';

  printf("digest=%s", buf);
}

}  // namespace libbyzea
