#include "Time.h"

namespace libbyzea {

long long clock_mhz = 0;

void init_clock_mhz() {
#ifndef USE_GETTIMEOFDAY
  struct timeval t0, t1;

  long long c0 = platform::cycle_count();
  gettimeofday(&t0, 0);
  sleep(1);
  long long c1 = platform::cycle_count();
  gettimeofday(&t1, 0);
  if (c1 < c0) {
    c1 += platform::MAX_CYCLE_COUNT - c0;
  }

  clock_mhz =
      (c1 - c0) / ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
#endif
}

}  // namespace libbyzea
