#ifndef _Partition_h
#define _Partition_h 1

#include <stddef.h>

#include "Message.h"
#include "Meta_data.h"
#include "libbyz.h"

namespace libbyzea {

constexpr int MAX_PARTITION_LEVELS =
    4;  //< Maximum number of partition tree levels.

#ifndef DYNAMIC_PARTITION_TREE

//
// Definitions for hierarchical state partitions.
//
constexpr int PChildren = 256;  // Number of children for non-leaf partitions.
constexpr int PLevels = 4;      // Number of levels in partition tree.

// Number of siblings at each level.
constexpr int PSize[] = {1, PChildren, PChildren, PChildren};

// Number of partitions at each level.
constexpr int PLevelSize[] = {1, PChildren, PChildren* PChildren,
                              PChildren* PChildren* PChildren};

// Number of blocks in a partition at each level
constexpr int PBlocks[] = {PChildren * PChildren * PChildren,
                           PChildren* PChildren, PChildren, 1};
#else

extern int PLevels;

constexpr int PChildren =
    (Max_message_size - sizeof(Meta_data_rep)) / sizeof(Part_info);

extern int PSize[MAX_PARTITION_LEVELS];

extern int PLevelSize[MAX_PARTITION_LEVELS];

extern int PBlocks[MAX_PARTITION_LEVELS];
#endif  // !DYNAMIC_PARTITION_TREE

namespace partition {
/**
 * @brief Return the number of meta-data partitions for a given block count.
 *
 * Return the total number of partitions that are active given the number of
 * state blocks.
 *
 * @param num_blocks the size of the state in blocks.
 * @return int the number of active partitions.
 */
inline int num_meta_for_blocks(int num_blocks) {
  int num_partitions = 0;
  int nodes_on_level = num_blocks;
  for (int level = PLevels - 1; level > 0; level--) {
    nodes_on_level = (nodes_on_level + PChildren - 1) / PChildren;
    num_partitions += nodes_on_level;
  }

  return num_partitions;
}

inline int blocks(int level) {
  return PBlocks[level + (MAX_PARTITION_LEVELS - PLevels)];
}

/**
 * @brief Initialize partition tree information
 */
int init(size_t mem_size);

}  // namespace partition
}  // namespace libbyzea

#endif /* _Partition_h */
