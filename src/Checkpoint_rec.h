#ifndef _Checkpoint_rec_h_
#define _Checkpoint_rec_h_ 1

#include <stddef.h>

#include "Part.h"
#include "Partition.h"
#include "map.h"
#include "th_assert.h"

namespace libbyzea {

// Key for partition map in checkpoint records
class PartKey {
 public:
  inline PartKey() {}
  inline PartKey(int l, int i) : level(l), index(i) {}
  inline PartKey(const PartKey& x) = default;

  inline PartKey& operator=(const PartKey& x) = default;

  inline int hash() const { return index << (PLevelSize[PLevels - 1] + level); }

  inline bool operator==(PartKey const& x) {
    return (level == x.level) && (index == x.index);
  }

  int level;
  int index;
};

// Checkpoint record
class Checkpoint_rec {
 public:
  static size_t memory_consumption();

  Checkpoint_rec();
  // Effects: Creates an empty checkpoint record.

  ~Checkpoint_rec();
  // Effects: Deletes record an all parts it contains

  void clear();
  // Effects: Deletes all parts in record and removes them.

  bool is_cleared();
  // Effects: Returns true iff Checkpoint record is not in use.

  void append(int l, int i, Part* p);
  // Requires: fetch(l, i) == 0
  // Effects: Appends partition index "i" at level "l" with value "p"
  // to the record.

  void appendr(int l, int i, Part* p);
  // Effects: Like append but without the requires clause. If fetch(l,
  // i) != 0 it retains the old mapping.

  Part* fetch(int l, int i);
  // Effects: If there is a partition with index "i" from level "l" in
  // this, returns a pointer to its information. Otherwise, returns 0.

  int num_entries() const;
  // Effects: Returns the number of entries in the record.

  class Iter {
   public:
    inline Iter(Checkpoint_rec* r) : g(r->parts) {}
    // Effects: Return an iterator for the partitions in r.

    inline bool get(int& level, int& index, Part*& p) {
      // Effects: Modifies "level", "index" and "p" to contain
      // information for the next partition in "r" and returns
      // true. Unless there are no more partitions in "r" in which case
      // it returns false.
      PartKey k;
      if (g.get(k, p)) {
        level = k.level;
        index = k.index;
        return true;
      }
      return false;
    }

   private:
    MapGenerator<PartKey, Part*> g;
  };
  friend class Iter;

  void print();
  // Effects: Prints description of this to stdout

  Digest sd;  // state digest at the time the checkpoint is taken

 private:
  // Map for partitions that were modified since this checkpoint was
  // taken and before the next checkpoint.
  Map<PartKey, Part*> parts;
};

inline size_t Checkpoint_rec::memory_consumption() {
  return (
      sizeof(libbyzea::Checkpoint_rec) +
      (256 * sizeof(libbyzea::Buckets<
                    libbyzea::HashPair<libbyzea::PartKey, libbyzea::Part*>>)));
}

inline Checkpoint_rec::Checkpoint_rec() : parts(256) {}

inline Checkpoint_rec::~Checkpoint_rec() { clear(); }

inline void Checkpoint_rec::append(int l, int i, Part* p) {
  th_assert(!parts.contains(PartKey(l, i)), "Invalid state");
  parts.add(PartKey(l, i), p);
}

inline void Checkpoint_rec::appendr(int l, int i, Part* p) {
  if (parts.contains(PartKey(l, i))) {
    return;
  }
  append(l, i, p);
}

inline Part* Checkpoint_rec::fetch(int l, int i) {
  Part* p;
  if (parts.find(PartKey(l, i), p)) {
    return p;
  }
  return 0;
}

inline bool Checkpoint_rec::is_cleared() { return sd.is_zero(); }

inline int Checkpoint_rec::num_entries() const { return parts.size(); }

}  // namespace libbyzea

#endif
