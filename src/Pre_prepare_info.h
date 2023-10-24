#include <cstddef>
#ifndef _Pre_prepare_info_h
#define _Pre_prepare_info_h 1

#include "Pre_prepare.h"
#include "types.h"

namespace libbyzea {

class Pre_prepare_info {
  //
  // Holds information about a pre-prepare.
  //
 public:
  Pre_prepare_info();
  // Effects: Creates an empty object.

  ~Pre_prepare_info();
  // Effects: Discard this and any pre-prepare message it may contain.

  void add(Pre_prepare* p);
  // Effects: Adds "p" to this.

  void add_complete(Pre_prepare* p);
  // Effects: Adds "p" to this and records that all the big reqs it
  // refers to are cached.

  Pre_prepare* pre_prepare() const;
  // Effects: If there is a pre-prepare message in this returns
  // it. Otherwise, returns 0.

  bool is_complete() const;
  // Effects: Returns true iff this contains a pre-prepare and all the
  // big requests it references are cached.

  void clear();
  // Effects: Makes this empty and deletes any pre-prepare in it.

  void zero();
  // Effects: Makes this empty without deleting any contained
  // pre-prepare.

  bool encode(FILE* o);
  bool decode(FILE* i);
  // Effects: Encodes and decodes object state from stream. Return
  // true if successful and false otherwise.

 private:
  Pre_prepare* pp;
};

inline Pre_prepare_info::Pre_prepare_info() { pp = nullptr; }

inline void Pre_prepare_info::zero() { pp = nullptr; }

inline void Pre_prepare_info::add_complete(Pre_prepare* p) {
  th_assert(pp == 0, "Invalid state");
  pp = p;
}

inline Pre_prepare* Pre_prepare_info::pre_prepare() const { return pp; }

inline void Pre_prepare_info::clear() {
  delete pp;
  pp = nullptr;
}

inline bool Pre_prepare_info::is_complete() const { return pp != nullptr; }

}  // namespace libbyzea

#endif  //_Pre_prepare_info_h
