#include <stdio.h>
#include <stdlib.h>

void *operator new(size_t sz) {
  fprintf(stderr, "new(%ld)\n", sz);
  return malloc(sz);
}

void operator delete(void *p) { return free(p); }
