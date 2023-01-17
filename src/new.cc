#include <stdio.h>
#include <stdlib.h>

void *operator new(size_t sz) { return malloc(sz); }

void *operator new[](size_t sz) { return operator new(sz); }

void operator delete(void *p, [[maybe_unused]] size_t sz) { return free(p); }

void operator delete[](void *p, size_t sz) { operator delete(p, sz); }

void operator delete(void *p) { free(p); }

void operator delete[](void *p) { operator delete(p); }
