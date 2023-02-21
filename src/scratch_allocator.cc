#include "scratch_allocator.h"

#include <stdint.h>

#include <cstring>

#include "Message.h"
#include "agreement_region.h"
#include "checkpoint_region.h"
#include "th_assert.h"

namespace libbyzea {
namespace scratch_allocator {

static constexpr int scratch_size = 3 * Max_message_size;
static char scratch_region[scratch_size] __attribute__((aligned(ALIGNMENT)));
static unsigned char scratch_bitmap = 0x0;
static constexpr unsigned char SCRATCH_MASK = 0x7;

size_t memory_demand() {
  return sizeof(scratch_size) + sizeof(scratch_region) +
         sizeof(scratch_bitmap) + sizeof(SCRATCH_MASK);
}

static inline void *scratch_end() {
  return (void *)(scratch_region + scratch_size);
}

void *malloc(size_t size) {
  th_assert(size <= Max_message_size,
            "Message larger than maximum message size");
  for (unsigned i = 0; i < (SCRATCH_MASK >> 1); i++) {
    char bit = 1 << i;
    if ((scratch_bitmap & bit) == 0) {
      scratch_bitmap |= bit;
      bzero(&scratch_region[i * Max_message_size], Max_message_size);
      return (void *)&scratch_region[i * Max_message_size];
    }
  }

  th_fail("Out of scratch memory");
}

void *realloc(void *msg, [[maybe_unused]] size_t size) { return msg; }

void free(void *msg, [[maybe_unused]] size_t size) {
  th_assert(msg >= (void *)scratch_region &&
                msg < (void *)(scratch_region + sizeof(scratch_region)),
            "Message is not part of scratch region");

  auto offset = (uintptr_t)msg - (uintptr_t)scratch_region;
  auto i = offset / Max_message_size;
  auto bit = 1 << i;
  if ((scratch_bitmap & bit) != bit) {
    th_fail("Double free detected");
  }
  scratch_bitmap &= ~bit;
}

bool is_in_scratch(void *msg) {
  return (msg >= scratch_region && msg < scratch_end());
}

}  // namespace scratch_allocator
}  // namespace libbyzea
