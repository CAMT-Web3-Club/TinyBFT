#ifndef _Cycle_counter_h
#define _Cycle_counter_h 1

#include "platform.h"

namespace libbyzea {

class Cycle_counter {
 public:
  Cycle_counter();
  // Effects: Create stopped counter with 0 cycles.

  void reset();
  // Effects: Reset counter to 0 and stop it.

  void start();
  // Effects: Start counter.

  void stop();
  // Effects: Stop counter and accumulate cycles since last started.

  long long elapsed();
  // Effects: Return cycles for which counter has run until now since
  // it was created or last reset.

  long long max_increment();
  // Effects: Return maximum number of cycles added to "accummulated" by
  // "stop()"

 private:
  long long c0, c1;
  long long accumulated;
  long long max_incr;
  bool running;

  // This variable should be set to the "average" value of c.elapsed()
  // after:
  // Cycle_counter c; c.start(); c.stop();
  //
  // The purpose is to avoid counting in the measurement overhead.
  static const long long calibration = 37;
};

inline void Cycle_counter::reset() {
  accumulated = 0;
  running = false;
  max_incr = 0;
}

inline Cycle_counter::Cycle_counter() { reset(); }

inline void Cycle_counter::start() {
  if (!running) {
    running = true;
    c0 = platform::cycle_count();
  }
}

inline void Cycle_counter::stop() {
  if (running) {
    running = false;
    c1 = platform::cycle_count();
    if (c1 < c0) {
      c1 += platform::MAX_CYCLE_COUNT - c0;
    }
    long long incr = c1 - c0 - calibration;
    if (incr > max_incr) max_incr = incr;
    accumulated += incr;
  }
}

inline long long Cycle_counter::elapsed() {
  if (running) {
    c1 = platform::cycle_count();
    if (c1 < c0) {
      c1 += platform::MAX_CYCLE_COUNT - c0;
    }
    return (accumulated + c1 - c0 - calibration);
  } else {
    return accumulated;
  }
}

inline long long Cycle_counter::max_increment() { return max_incr; }

}  // namespace libbyzea

#endif  // _Cycle_counter_h
