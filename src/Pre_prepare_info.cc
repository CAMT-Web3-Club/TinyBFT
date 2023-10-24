#include "Pre_prepare_info.h"

#include "Pre_prepare.h"
#include "Replica.h"

namespace libbyzea {

Pre_prepare_info::~Pre_prepare_info() { delete pp; }

void Pre_prepare_info::add(Pre_prepare* p) {
  th_assert(pp == 0, "Invalid state");
  pp = p;
}

bool Pre_prepare_info::encode(FILE* o) {
  bool hpp = pp != 0;
  size_t sz = fwrite(&hpp, sizeof(bool), 1, o);
  bool ret = true;
  if (hpp) ret = pp->encode(o);
  return ret & (sz == 1);
}

bool Pre_prepare_info::decode(FILE* i) {
  bool hpp;
  size_t sz = fread(&hpp, sizeof(bool), 1, i);
  bool ret = true;
  if (hpp) {
    pp = (Pre_prepare*)new Message;
    ret &= pp->decode(i);
  }
  return ret & (sz == 1);
}

}  // namespace libbyzea
