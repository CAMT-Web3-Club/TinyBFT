#include "simple_log_allocator.h"

#include <stdlib.h>

#include "Log_allocator.h"
#include "Message.h"
#include "parameters.h"
#include "th_assert.h"

namespace libbyzea {

SimpleLogAllocator::SimpleLogAllocator() {}

char *SimpleLogAllocator::malloc(int sz) {
  void *buf;
  int err;

  err = posix_memalign(&buf, ALIGNMENT, sz);
  th_assert(err == 0, "failed to allocate log memory");
  return reinterpret_cast<char *>(buf);
}

void SimpleLogAllocator::free(char *p, [[maybe_unused]] int sz) { ::free(p); }

bool SimpleLogAllocator::realloc(char *p, int osz, int nsz) {
  if (osz < nsz) {
    return false;
  }

  void *tmp = ::realloc(p, nsz);
  if (tmp == nullptr) {
    return false;
  }
  th_assert(p == tmp, "p has been reallocated to another address");

  return true;
}
void SimpleLogAllocator::debug_print() {}

}  // namespace libbyzea
