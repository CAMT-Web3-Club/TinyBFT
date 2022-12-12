#ifndef _LIBBYZEA_LOG_RING_ALLOCATOR_H_
#define _LIBBYZEA_LOG_RING_ALLOCATOR_H_

#include <stdint.h>

namespace libbyzea {

class SimpleLogAllocator {
 public:
  SimpleLogAllocator();

  char *malloc(int sz);
  void free(char *p, int sz);
  bool realloc(char *p, int osz, int nsz);
  void debug_print();
};

}  // namespace libbyzea
#endif  // !_LIBBYZEA_LOG_RING_ALLOCATOR_H_
