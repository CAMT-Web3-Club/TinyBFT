#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mem_statistics.h"

#define MAX_HEAP_SIZE 1342177280L

static void *heap = NULL;
static intptr_t heap_pos = 0;

/**
 * Returns x aligned to the next multiple of alignment.
 */
static inline intptr_t align_to(intptr_t x, int alignment) {
  if (x % alignment == 0) {
    return x;
  }
  x += alignment - (x % alignment);
  return x;
}

static inline void print_alloc_info(const char *fmt, size_t size) {
  static char string_buf[4096];

  // Cannot use printf here, since it might allocate memory using malloc itself.
  int n = snprintf(string_buf, 4096, fmt, size);
  if (n >= 4096) {
    n = 4095;
  }
  write(STDERR_FILENO, string_buf, n);
  fsync(STDERR_FILENO);
}

void *malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  if (heap == NULL) {
    heap = mmap(NULL, MAX_HEAP_SIZE, PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (heap == MAP_FAILED) {
      errno = ENOMEM;
      return NULL;
    }
  }

  intptr_t chunk_size =
      align_to(size + 2 * sizeof(intptr_t), sizeof(long double));
  if (heap_pos + chunk_size >= MAX_HEAP_SIZE) {
    errno = ENOMEM;
    return NULL;
  }

  MEMSTATS_TRACK_CHANGE(size);
  intptr_t *ret = (intptr_t *)((intptr_t)heap + heap_pos);
  *ret = chunk_size;
  ret++;
  *ret = (intptr_t)size;
  ret++;
  heap_pos += chunk_size;

  return (void *)ret;
}

void *realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return malloc(size);
  }

  intptr_t *slice = (intptr_t *)ptr;
  slice--;
  intptr_t alloc_size = *slice;
  slice--;
  intptr_t slice_size = *slice;
  if (size <= (size_t)alloc_size) {
    *(slice + 1) = size;
    intptr_t diff = alloc_size - (intptr_t)size;
    MEMSTATS_TRACK_CHANGE(diff);
    return ptr;
  }

  /*
   * If we are at the end of the heap, we can just increase the slice size.
   */
  intptr_t end = (intptr_t)slice + alloc_size;
  if (end == ((intptr_t)heap + heap_pos)) {
    intptr_t diff = size - slice_size;
    if (heap_pos + align_to(diff, sizeof(long double)) > MAX_HEAP_SIZE) {
      errno = ENOMEM;
      return NULL;
    }

    MEMSTATS_TRACK_CHANGE(diff);
    heap_pos += align_to(diff, sizeof(long double));
    *slice += align_to(diff, sizeof(long double));
    *(slice + 1) = (intptr_t)size;
    return ptr;
  }

  void *tmp = malloc(size);
  if (tmp == NULL) {
    return NULL;
  }
  memcpy(tmp, ptr, slice_size);
  free(ptr);

  return tmp;
}

void *calloc(size_t nmemb, size_t size) {
  size_t full_size;
  if (__builtin_mul_overflow(nmemb, size, &full_size)) {
    errno = EINVAL;
    return NULL;
  }
  void *addr = malloc(full_size);
  if (addr == NULL) {
    return addr;
  }
  bzero(addr, full_size);

  return addr;
}

void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  intptr_t *slice = (intptr_t *)ptr;
  slice--;
  intptr_t size = *slice;
  slice--;
  intptr_t chunk_size = *slice;
  uintptr_t addr = (uintptr_t)slice;
  if ((addr + chunk_size) == ((uintptr_t)heap + heap_pos)) {
    heap_pos -= chunk_size;
  }

  MEMSTATS_TRACK_CHANGE(-size);
}
