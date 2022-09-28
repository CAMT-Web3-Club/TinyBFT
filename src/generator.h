#ifndef _GENERATOR_H
#define _GENERATOR_H

#pragma interface

#include "basic.h"

namespace libbyzea {

template <class T>
class Generator {
 public:
  Generator() {}
  virtual bool get(T&) = 0;
  virtual ~Generator(){};
};

}  // namespace libbyzea

#endif /* _GENERATOR_H */
