#include "platform.h"

namespace libbyzea {
namespace platform {

#if defined(ESP_PLATFORM) && defined(__riscv)
/**
 * @brief Performance Counter Event Register
 *
 * Register selecting the performance counter events that increment the
 * performance counter.
 */
union mpcer {
  struct {
    uint32_t cycles : 1, instructions : 1, load_hazards : 1, jump_hazards : 1,
        idle : 1, loads : 1, stores : 1, unconditional_jumps : 1, branches : 1,
        branch_taken : 1, compressed_instructions : 1, : 21;
  } bits;
  uint32_t value;
} __attribute__((packed));

/**
 * @brief Performance Counter Mode Register
 */
union mpcmr {
  struct {
    uint32_t enabled : 1, saturation_control : 1, : 30;
  } bits;
  uint32_t value;
} __attribute__((packed));

enum class saturation_control : uint8_t {
  WRAP_AROUND = 0x0,
  HALT = 0x1,
};

static inline void write_csr(csr csr, uint32_t value) {
  __asm__("csrw %0, %1" : : "i"(csr), "r"(value));
}

void initialize() {
  union mpcer event;
  event.value = 0;
  event.bits.cycles = 1;
  write_csr(csr::MPCER, event.value);

  // The performance counter halts when hitting its maximum value by default. We
  // use the wrap-around mode here since that is closer to the RTDSC behaviour.
  union mpcmr mode;
  mode.value = 0;
  mode.bits.enabled = 1;
  mode.bits.saturation_control =
      static_cast<uint32_t>(saturation_control::WRAP_AROUND);
  write_csr(csr::MPCMR, mode.value);
}

#else

void initialize(){};

#endif  // ESP_PLATFORM && __riscv

}  // namespace platform
}  // namespace libbyzea
