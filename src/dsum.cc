#include "dsum.h"

#include "th_assert.h"

namespace libbyzea {

DSum* DSum::M = nullptr;

void DSum::init(const std::string& modulus) {
  M = new DSum;
  int err = mbedtls_mpi_read_string(&DSum::M->sum, 16, modulus.c_str());
  th_assert(!err, "failed to initialize digest sum modulus");
  static_assert(sizeof(Digest) % sizeof(mbedtls_mpi_uint) == 0,
                "Invalid assumption: sizeof(Digest)%sizeof(mp_limb_t)");
}

void DSum::add(Digest& d) {
  mbedtls_mpi digest;
  mbedtls_mpi_init(&digest);
  const uint8_t* buf = reinterpret_cast<const uint8_t*>(d.digest());
  int ret = mbedtls_mpi_read_binary(&digest, buf, sizeof(d));
  th_assert(ret == 0, "failed to copy digest to multi-precision structure");

  ret = mbedtls_mpi_add_mpi(&sum, &sum, &digest);
  th_assert(ret == 0, "failed to add sum and digest");
  if (mbedtls_mpi_cmp_mpi(&sum, &M->sum) >= 0) {
    mbedtls_mpi_sub_mpi(&sum, &sum, &M->sum);
  }
  th_assert(mbedtls_mpi_size(&sum) <= nbytes, "digest sum is too large");

  mbedtls_mpi_free(&digest);
}

void DSum::sub(Digest& d) {
  mbedtls_mpi digest;
  mbedtls_mpi_init(&digest);
  const uint8_t* buf = reinterpret_cast<const uint8_t*>(d.digest());
  int ret = mbedtls_mpi_read_binary(&digest, buf, sizeof(d));
  th_assert(ret == 0, "failed to copy digest to multi-precision structure");

  if (mbedtls_mpi_cmp_mpi(&sum, &digest) < 0) {
    ret = mbedtls_mpi_add_mpi(&sum, &sum, &M->sum);
    th_assert(ret == 0, "failed mod M");
  }
  ret = mbedtls_mpi_sub_mpi(&sum, &sum, &digest);
  th_assert(ret == 0, "failed to subtract digest from digest sum");
  th_assert(mbedtls_mpi_cmp_int(&sum, -1) > 0, "sum should be positive");

  mbedtls_mpi_free(&digest);
}

void DSum::print() {
  int err = mbedtls_mpi_write_file("DSum:0x", &sum, 16, stderr);
  fprintf(stderr, "\n");
  th_assert(!err, "failed to write DSum to stderr");
}

}  // namespace libbyzea
