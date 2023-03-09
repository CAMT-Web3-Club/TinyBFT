#ifndef _LIBBYZ_PLATFORM_H_
#define _LIBBYZ_PLATFORM_H_

#include <stdint.h>
#include <unistd.h>

#ifdef ESP_PLATFORM
#include "esp_timer.h"
#endif

#if !defined(__i386__) && !defined(__x86_64__) && \
    (!defined(ESP_PLATFORM) || !defined(__riscv))
#error "Unsupported Hardware Platform"
#endif

/**
 * @brief Platform dependent functions.
 */

namespace libbyzea {
namespace platform {

#if defined(__i386__) || defined(__x86_64__)
constexpr auto MAX_CYCLE_COUNT = static_cast<uint64_t>(UINT64_MAX);
#elif defined(ESP_PLATFORM)
constexpr auto MAX_CYCLE_COUNT = static_cast<uint64_t>(UINT32_MAX);
#endif  // ESP_PLATFORM

#if defined(ESP_PLATFORM) && defined(__riscv)
enum class csr : uint32_t {
  MPCER = 0x7e0,
  MPCMR = 0x7e1,
  MPCCR = 0x7e2,
};
#endif

/**
 * @brief Initialize the hardware platform
 *
 */
void initialize();

union cycle_count {
  struct {
    uint32_t low;
    uint32_t high;
  } words;
  uint64_t value;
};

/**
 * @brief Get the current value of the platform's cycle counter.
 *
 * @return uint64_t the current value of the cycle counter.
 */
inline uint64_t cycle_count() {
  union cycle_count count;

#if defined(__i386__) || defined(__x86_64__)
  unsigned int aux;
  count.value = __builtin_ia32_rdtscp(&aux);
#elif defined(ESP_PLATFORM) && defined(__riscv)
  count.words.high = 0;
  __asm__("csrr %0, %1" : "=r"(count.words.low) : "i"(csr::MPCCR));
#endif

  return count.value;
}

/**
 * @brief Delay execution for an number of seconds.
 *
 * Delay execution for a number of seconds in such a way that the cycle counter
 * register still counts clock cycles.
 *
 * @param seconds the number of seconds to delay execution.
 */
inline void delay(unsigned seconds) {
#if defined(ESP_PLATFORM) && defined(__riscv)
  // XXX: The clock cycle count register does not increase during sleep on the
  // ESP32c3, so we have to use a busy loop here.
  int64_t time = esp_timer_get_time();
  int64_t delay = seconds * 1000 * 1000 + time;
  while (time < delay) {
    time = esp_timer_get_time();
  }
#else
  sleep(seconds);
#endif
}

}  // namespace platform
}  // namespace libbyzea

#endif  // !_LIBBYZ_HARDWARE_CYCLE_COUNTER_H_
