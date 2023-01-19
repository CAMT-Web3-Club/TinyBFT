#ifndef LIBBYZEA_DSUM_H_
#define LIBBYZEA_DSUM_H_

#include <mbedtls/bignum.h>

#include <string>

#include "Digest.h"
#include "mem_statistics.h"

namespace libbyzea {
//
// Sums of digests modulo a large integer
//
struct DSum {
  static const int nbits = 256;
  static const int mp_limb_bits = sizeof(mbedtls_mpi_uint) * 8;
  static const int nlimbs = (nbits + mp_limb_bits - 1) / mp_limb_bits;
  static const int nbytes = nlimbs * sizeof(mbedtls_mpi_uint);
  static DSum* M;  // Modulus for sums must be at most nbits-1 long.

  mbedtls_mpi sum;
  //  char dummy[56];

  static void init(const std::string& modulus);

  inline DSum() {
    MEMSTATS_CALL_STACK_PUSH(DSum::DSum);
    mbedtls_mpi_init(&sum);
    mbedtls_mpi_grow(&sum, nlimbs);
    mbedtls_mpi_lset(&sum, 0);
    MEMSTATS_CALL_STACK_POP();
  }
  // Effects: Creates a new sum object with value 0

  inline DSum(DSum const& other) {
    MEMSTATS_CALL_STACK_PUSH(DSum::DSum);
    mbedtls_mpi_init(&sum);
    mbedtls_mpi_copy(&sum, &other.sum);
    MEMSTATS_CALL_STACK_POP();
  }

  inline ~DSum() { mbedtls_mpi_free(&sum); }

  inline DSum& operator=(DSum const& other) {
    if (this == &other) {
      return *this;
    }

    mbedtls_mpi_copy(&sum, &other.sum);

    return *this;
  }

  void add(Digest& d);
  // Effects: adds "d" to this

  void sub(Digest& d);
  // Effects: subtracts "d" from this.

  void print();
  // Effects: prints the sum's current value.
};

}  // namespace libbyzea
#endif  // !LIBBYZEA_DSUM_H_
