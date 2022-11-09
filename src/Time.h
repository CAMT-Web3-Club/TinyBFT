#ifndef _Time_h
#define _Time_h 1

/*
 * Definitions of various types.
 */
#include <limits.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef USE_GETTIMEOFDAY

namespace libbyzea {

typedef struct timeval Time;

extern long long clock_mhz;

inline Time currentTime() {
  Time t;
  int ret = gettimeofday(&t, 0);
  th_assert(ret == 0, "gettimeofday failed");
  return t;
}

inline Time zeroTime() {
  Time t;
  t.tv_sec = 0;
  t.tv_usec = 0;
  return t;
}

inline bool equalTime(Time t1, Time t2) {
  return (t1.tv_sec == t2.tv_sec && t1.tv_usec == t2.tv_usec);
}

inline long long diffTime(Time t1, Time t2) {
  // t1-t2 in microseconds.
  return (((unsigned long long)(t1.tv_sec - t2.tv_sec)) << 20) +
         (t1.tv_usec - t2.tv_usec);
}

inline bool lessThanTime(Time t1, Time t2) {
  return t1.tv_sec < t2.tv_sec ||
         (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec);
}

void init_clock_mhz();

}  // namespace libbyzea

#else

#include "Cycle_counter.h"

namespace libbyzea {

typedef long long Time;

extern long long clock_mhz;
// Clock frequency in MHz

void init_clock_mhz();
// Effects: Initialize "clock_mhz".

inline Time currentTime() { return platform::cycle_count(); }

inline Time zeroTime() { return 0; }

inline long long diffTime(Time t1, Time t2) { return (t1 - t2) / clock_mhz; }

inline bool lessThanTime(Time t1, Time t2) { return t1 < t2; }

inline bool equalTime(Time t1, Time t2) { return (t1 == t2); }

}  // namespace libbyzea

#endif

#endif  // _Time_h
