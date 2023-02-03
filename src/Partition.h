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

/**
 * @brief Return the number of meta-data partitions for a given block count.
 *
 * Return the total number of partitions that are active given the number of
 * state blocks.
 *
 * @param num_blocks the size of the state in blocks.
 * @return int the number of active partitions.
 */
inline int num_meta_partitions_for_blocks(int num_blocks) {
  int num_partitions = 0;
  int nodes_on_level = num_blocks;
  for (int level = PLevels - 1; level > 0; level--) {
    nodes_on_level = (nodes_on_level + PChildren - 1) / PChildren;
    num_partitions += nodes_on_level;
  }

  return num_partitions;
}

}  // namespace libbyzea

#endif /* _Partition_h */
