#include "scratch_allocator.h"

#include <stdint.h>

#include <cstring>

#include "Message.h"
#include "agreement_region.h"
#include "checkpoint_region.h"
#include "th_assert.h"
#include <assert.h>

namespace libbyzea {
namespace scratch_allocator {
#define SLOTS 3
static constexpr int scratch_size = SLOTS * Max_message_size;
static char scratch_region[scratch_size] __attribute__((aligned(ALIGNMENT)));
static unsigned char scratch_bitmap[(SLOTS + 7) / 8];

size_t memory_demand() {
  return sizeof(scratch_size) + sizeof(scratch_region) +
         sizeof(scratch_bitmap);
}

static inline void *scratch_end() {
  return (void *)(scratch_region + scratch_size);
}

static inline char get_bit(int i) {
  char map = scratch_bitmap[i / 8];
  char bit = i % 8;
  return (map >> bit) & 0x1;
}

static inline void set_bit(int i) {
  char bit = i % 8;
  scratch_bitmap[i / 8] |= (1 << bit);
}

static inline void unset_bit(int i) {
  char bit = i % 8;
  scratch_bitmap[i / 8] &= ~(1 << bit);
}

void *malloc(size_t size) {
  th_assert(size <= Max_message_size,
            "Message larger than maximum message size");
  for (unsigned i = 0; i < SLOTS; i++) {
    if (get_bit(i) == 0) {
      bzero(&scratch_region[i * Max_message_size], Max_message_size);
      set_bit(i);
      return (void *)&scratch_region[i * Max_message_size];
    }
  }

  th_fail("Out of scratch memory");
  return NULL;
}

void *realloc(void *msg, [[maybe_unused]] size_t size) { return msg; }

void free(void *msg, [[maybe_unused]] size_t size) {
  th_assert(msg >= (void *)scratch_region &&
                msg < (void *)(scratch_region + sizeof(scratch_region)), "Message is not part of scratch region");

  auto offset = (uintptr_t)msg - (uintptr_t)scratch_region;
  auto i = offset / Max_message_size;
  th_assert(get_bit(i) != 0, "Double free detected");
  unset_bit(i);
  th_assert(get_bit(i) == 0, "Free failed");
}

bool is_in_scratch(void *msg) {
  return (msg >= scratch_region && msg < scratch_end());
}

}  // namespace scratch_allocator
}  // namespace libbyzea
