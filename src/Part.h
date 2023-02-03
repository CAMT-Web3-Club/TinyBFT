#ifndef _Part_h_
#define _Part_h_ 1

#include <string.h>

#include "Digest.h"
#include "State_defs.h"
#include "libbyz.h"
#include "types.h"

namespace libbyzea {
//
// The memory managed by the state abstraction is partitioned into
// blocks.
//
struct Block {
#ifdef NO_STATE_TRANSLATION
  char data[Block_size];
#else
  char* data;
  int size;
#endif

  inline Block() {
#ifndef NO_STATE_TRANSLATION
    data = nullptr;
    size = 0;
#endif
  }

  inline Block(Block const& other) {
#ifndef NO_STATE_TRANSLATION
    size = other.size;
    data = new char[size];
    memcpy(data, other.data, size);
#else
    memcpy(data, other.data, Block_size);
#endif
  }

#ifndef NO_STATE_TRANSLATION
  inline ~Block() {
    if (data) delete[] data;
  }

  inline void Block::init_from_ptr(char* ptr, int psize) {
    if (data) delete[] data;
    data = ptr;
    size = psize;
  }

#endif

  inline Block& operator=(Block const& other) {
    if (this == &other) return *this;
#ifndef NO_STATE_TRANSLATION
    if (size != other.size) {
      if (data) delete[] data;
      data = new char[other.size];
    }
    size = other.size;
    memcpy(data, other.data, other.size);
#else
    memcpy(data, other.data, Block_size);
#endif
    return *this;
  }

  inline Block& operator=(char const* other) {
    if (this->data == other) return *this;
#ifndef NO_STATE_TRANSLATION
    if (size != Block_size) {
      if (data) delete[] data;
      data = new char[Block_size];
    }
    size = Block_size;
#endif
    memcpy(data, other, Block_size);
    return *this;
  }
};

// Blocks are grouped into partitions that form a hierarchy.
// Part contains information about one such partition.
struct Part {
  Seqno lm;  // Sequence number of last checkpoint that modified partition
  Digest d;  // Digest of partition

#ifndef NO_STATE_TRANSLATION
  int size;  // Size of object for level 'PLevels-1'
#endif

  Part() { lm = 0; }
};

// Copy of leaf partition (used in checkpoint records)
struct BlockCopy : public Part {
  Block data;  // Copy of data at the time the checkpoint was taken

  BlockCopy() : Part() {}

#ifndef NO_STATE_TRANSLATION
  BlockCopy(char* d, int sz) : Part() { data.init_from_ptr(d, sz); }
#endif
};
}  // namespace libbyzea

#endif
