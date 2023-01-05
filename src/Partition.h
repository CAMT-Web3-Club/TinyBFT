#ifndef _Partition_h
#define _Partition_h 1

#include "libbyz.h"

namespace libbyzea {

//
// Definitions for hierarchical state partitions.
//
#ifndef PARTITION_TREE_LEVELS
#define PARTITION_TREE_LEVELS 4
#endif

#ifndef PARTITION_TREE_CHILDREN
#define PARTITION_TREE_CHILDREN 256
#endif

constexpr int PChildren =
    PARTITION_TREE_CHILDREN;  // Number of children for non-leaf partitions.
constexpr int PLevels =
    PARTITION_TREE_LEVELS;  // Number of levels in partition tree.
constexpr int MAX_PARTITION_LEVELS =
    4;  //< Maximum number of partition tree levels.
static_assert(PLevels >= 2 && PLevels <= MAX_PARTITION_LEVELS,
              "Partition Tree Level must be between 2 and 4");

// Number of siblings at each level.
constexpr int PSize[] = {1, PChildren, PChildren* PChildren,
                         PChildren* PChildren* PChildren};

// Number of partitions at each level.
constexpr int PLevelSize[] = {1, PChildren, PChildren* PChildren,
                              PChildren* PChildren* PChildren};

// Number of blocks in a partition at each level
constexpr int PBlocks[] = {PChildren * PChildren * PChildren,
                           PChildren* PChildren, PChildren, 1};

inline constexpr int partition_blocks(int level) {
  return PBlocks[level + (MAX_PARTITION_LEVELS - PLevels)];
}

}  // namespace libbyzea

#endif /* _Partition_h */
