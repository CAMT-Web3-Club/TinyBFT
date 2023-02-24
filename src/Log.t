#include <stdlib.h>

#include "Log.h"
#include "mem_statistics.h"
#include "th_assert.h"
#include "types.h"

namespace libbyzea {

template <class T>
size_t Log<T>::memory_consumption(size_t sz) {
  return (sz * (sizeof(T) + T::memory_consumption()));
}

template <class T>
Log<T>::Log(MEM_STATS_PARAM int sz, Seqno h) : head(h), max_size(sz) {
  elems = new T[sz];
  mask = max_size - 1;
  MEM_STATS_GUARD_POP();
}

template <class T>
Log<T>::~Log() {
  delete[] elems;
}

template <class T>
void Log<T>::clear(Seqno h) {
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  for (int i = 0; i < max_size; i++) elems[i].clear();
  MEMSTATS_RESTORE_MEM_TYPE();

  head = h;
}

template <class T>
T &Log<T>::fetch(Seqno seqno) {
  th_assert(within_range(seqno), "Invalid argument\n");
  return elems[mod(seqno)];
}

template <class T>
void Log<T>::truncate(Seqno new_head) {
  MEMSTATS_MEM_TYPE_VAR
  if (new_head <= head) return;

  int i = head;
  int max = new_head;
  if (new_head - head >= max_size) {
    i = 0;
    max = max_size;
  }

  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  for (; i < max; i++) {
    elems[mod(i)].clear();
  }
  MEMSTATS_RESTORE_MEM_TYPE();

  head = new_head;
}

}  // namespace libbyzea
