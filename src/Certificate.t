#include <typeinfo>

#include "Certificate.h"
#include "Node.h"
#include "scratch_allocator.h"

// TODO: Certificates already only use f+1 distinct messages to be considered
// correct. Therefore, we can simply use cur_size as the slot and reduce number
// of entries in static allocators to f+1.

namespace libbyzea {
template <class T>
size_t Certificate<T>::memory_consumption() {
  return sizeof(Certificate<T>) + 2 * (sizeof(void *) + sizeof(int)) +
         ((Max_num_replicas + ChunkBits - 1) / ChunkBits);
}

#ifdef PRINT_MEM_STATISTICS
template <class T>
Certificate<T>::Certificate(int comp)
    : Certificate<T>(MEM_STATS_ARG_INIT_PUSH(Certificate<T>) comp) {}
#endif

template <class T>
Certificate<T>::Certificate(MEM_STATS_PARAM int comp) : bmap(Max_num_replicas) {
  max_size = node->f() + 1;
  vals = new Message_val[max_size];
  cur_size = 0;
  correct = node->f() + 1;
  complete = (comp == 0) ? node->f() * 2 + 1 : comp;
  c = nullptr;
#ifdef STATIC_LOG_ALLOCATOR
  correct_id = -1;
#endif
  mym = nullptr;
  MEM_STATS_GUARD_POP();
}

template <class T>
Certificate<T>::~Certificate() {
  delete[] vals;
}

template <class T>
bool Certificate<T>::add(T *m) {
  const int id = m->id();
  if (node->is_replica(id) && !bmap.test(id)) {
    // "m" was sent by a replica that does not have a message in
    // the certificate
    if ((c == nullptr || (c->count < complete && c->m->match(m))) &&
        m->verify()) {
      // add "m" to the certificate
      th_assert(id != node->id(),
                "verify should return false for messages from self");

      bmap.set(id);
      if (c != nullptr) {
        c->count++;
        if (!c->m->full() && m->full()) {
          // if c->m is not full and m is, replace c->m
#ifdef STATIC_LOG_ALLOCATOR
          m = m->persist(correct_id);
#else
          delete c->m;
#endif
          c->m = m;
        } else {
#ifndef STATIC_LOG_ALLOCATOR
          delete m;
#endif
        }
        return true;
      }

      // Check if there is a value that matches "m"
      int i;
      for (i = 0; i < cur_size; i++) {
        Message_val &val = vals[i];
        if (val.m->match(m)) {
          val.count++;
          if (val.count >= correct) {
            c = vals + i;
#ifdef STATIC_LOG_ALLOCATOR
            correct_id = i;
#endif
          }
          if (!val.m->full() && m->full()) {
            // if val.m is not full and m is, replace val.m
#ifdef STATIC_LOG_ALLOCATOR
            m = m->persist(i);
#else
            delete val.m;
#endif
            val.m = m;
          } else {
#ifndef STATIC_LOG_ALLOCATOR
            delete m;
#endif
          }
          return true;
        }
      }

      // "m" has a new value.
      if (cur_size < max_size) {
#ifdef STATIC_LOG_ALLOCATOR
        m = m->persist(cur_size);
#endif
        vals[cur_size].m = m;
        vals[cur_size++].count = 1;
        return true;
      } else {
        // Should only happen for replies to read-only requests.
        fprintf(stderr, "More than f+1 distinct values in certificate");
        clear();
      }

    } else {
      if (m->verify()) bmap.set(id);
    }
  }
#ifdef STATIC_LOG_ALLOCATOR
  if (scratch_allocator::is_in_scratch(m->contents())) {
    delete m;
  }
#else
  delete m;
#endif
  return false;
}

template <class T>
bool Certificate<T>::add_mine(T *m) {
  th_assert(m->id() == node->id(), "Invalid argument");
  th_assert(m->full(), "Invalid argument");

  if (c != nullptr && !c->m->match(m)) {
    fprintf(
        stderr,
        "Node is faulty, more than f faulty replicas or faulty primary %s\n",
        m->stag());
    delete m;
    return false;
  }

  if (c == nullptr) {
    // Set m to be the correct value.
    int i;
    for (i = 0; i < cur_size; i++) {
      if (vals[i].m->match(m)) {
        c = vals + i;
#ifdef STATIC_LOG_ALLOCATOR
        correct_id = i;
#endif
        break;
      }
    }

    if (c == nullptr) {
      c = vals;
      vals->count = 0;
#ifdef STATIC_LOG_ALLOCATOR
      correct_id = 0;
#endif
    }
  }

  if (c->m == nullptr) {
    th_assert(cur_size == 0, "Invalid state");
    cur_size = 1;
  }

#ifdef STATIC_LOG_ALLOCATOR
  m = m->persist(correct_id);
  bmap.set(node->id());
#endif

#ifndef STATIC_LOG_ALLOCATOR
  delete c->m;
#endif
  c->m = m;
  c->count++;
  mym = m;
  t_sent = currentTime();
  return true;
}

template <class T>
void Certificate<T>::mark_stale() {
  if (!is_complete()) {
    int i = 0;
    int old_cur_size = cur_size;
    if (mym) {
      th_assert(mym == c->m, "Broken invariant");
      c->m = 0;
      c->count = 0;
      c = vals;
      c->m = mym;
      c->count = 1;
      i = 1;
    } else {
      c = 0;
    }
    cur_size = i;

    for (; i < old_cur_size; i++) vals[i].clear();
    bmap.clear();
  }
}

template <class T>
T *Certificate<T>::cvalue_clear() {
  if (c == nullptr) {
    return 0;
  }

  T *ret = c->m;
  c->m = nullptr;
#ifdef STATIC_LOG_ALLOCATOR
  correct_id = -1;
#endif
  for (int i = 0; i < cur_size; i++) {
    if (vals[i].m == ret) {
      vals[i].m = nullptr;
    }
  }
  clear();

  return ret;
}

template <class T>
bool Certificate<T>::encode(FILE *o) {
  bool ret = bmap.encode(o);

  size_t sz = fwrite(&max_size, sizeof(int), 1, o);
  sz += fwrite(&cur_size, sizeof(int), 1, o);
  for (int i = 0; i < cur_size; i++) {
    int vcount = vals[i].count;
    sz += fwrite(&vcount, sizeof(int), 1, o);
    if (vcount) {
      ret &= vals[i].m->encode(o);
    }
  }

  sz += fwrite(&complete, sizeof(int), 1, o);

  int cindex = (c != 0) ? c - vals : -1;
  sz += fwrite(&cindex, sizeof(int), 1, o);

  bool hmym = mym != 0;
  sz += fwrite(&hmym, sizeof(bool), 1, o);

  return ret & (sz == 5U + cur_size);
}

template <class T>
bool Certificate<T>::decode(FILE *in) {
  bool ret = bmap.decode(in);

  size_t sz = fread(&max_size, sizeof(int), 1, in);
  delete[] vals;

  vals = new Message_val[max_size];

  sz += fread(&cur_size, sizeof(int), 1, in);
  if (cur_size < 0 || cur_size >= max_size) return false;

  for (int i = 0; i < cur_size; i++) {
    sz += fread(&vals[i].count, sizeof(int), 1, in);
    if (vals[i].count < 0 || vals[i].count > node->n()) return false;

    if (vals[i].count) {
      vals[i].m = (T *)new Message;
      ret &= vals[i].m->decode(in);
    }
  }

  sz += fread(&complete, sizeof(int), 1, in);
  correct = node->f() + 1;

  int cindex;
  sz += fread(&cindex, sizeof(int), 1, in);

  bool hmym;
  sz += fread(&hmym, sizeof(bool), 1, in);

  if (cindex == -1) {
    c = 0;
    mym = 0;
  } else {
    if (cindex < 0 || cindex > cur_size) return false;
    c = vals + cindex;

    if (hmym) mym = c->m;
  }

  t_sent = zeroTime();

  return ret & (sz == 5U + cur_size);
}

}  // namespace libbyzea
