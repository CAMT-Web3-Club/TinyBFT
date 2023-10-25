#include "Prepared_cert.h"

#include <cstddef>

#include "Certificate.t"
#include "Node.h"

namespace libbyzea {

template class Certificate<Prepare>;

size_t Prepared_cert::memory_consumption() {
  return Certificate<Prepare>::memory_consumption();
}

Prepared_cert::Prepared_cert()
    : pc(node->f() * 2), pp(nullptr), primary(false) {}

Prepared_cert::~Prepared_cert() {
  if (pp != nullptr) {
    delete pp;
  }
}

bool Prepared_cert::is_pp_correct() {
  if (pp != nullptr) {
    Certificate<Prepare>::Val_iter viter(&pc);
    int vc;
    Prepare* val;
    while (viter.get(val, vc)) {
      if (vc >= node->f() && pp->match(val)) {
        return true;
      }
    }
  }
  return false;
}

bool Prepared_cert::add(Pre_prepare* m) {
  if (pp == nullptr) {
    Prepare* p = pc.mine();

    if (p == nullptr) {
      if (m->verify()) {
#ifdef STATIC_LOG_ALLOCATOR
        m = m->persist();
#endif
        pp = m;
        return true;
      }

      if (m->verify(Pre_prepare::NRC)) {
        // Check if there is some value that matches pp and has f
        // senders.
        Certificate<Prepare>::Val_iter viter(&pc);
        int vc;
        Prepare* val;
        while (viter.get(val, vc)) {
          if (vc >= node->f() && m->match(val)) {
#ifdef STATIC_LOG_ALLOCATOR
            m = m->persist();
#endif
            pp = m;
            return true;
          }
        }
      }
    } else {
      // If we sent a prepare, we only accept a matching pre-prepare.
      if (m->match(p) && m->verify(Pre_prepare::NRC)) {
#ifdef STATIC_LOG_ALLOCATOR
        m = m->persist();
#endif
        pp = m;
        return true;
      }
    }
  }
  delete m;
  return false;
}

bool Prepared_cert::encode(FILE* o) {
  bool ret = pc.encode(o);
  ret &= pp->encode(o);
  int sz = fwrite(&primary, sizeof(bool), 1, o);
  return ret & (sz == 1);
}

bool Prepared_cert::decode(FILE* i) {
  th_assert(pp != nullptr, "Invalid state");

  bool ret = pc.decode(i);
  ret &= pp->decode(i);
  int sz = fread(&primary, sizeof(bool), 1, i);
  t_sent = zeroTime();

  return ret & (sz == 1);
}

bool Prepared_cert::is_empty() const { return pp == nullptr && pc.is_empty(); }

}  // namespace libbyzea
