#include "Partition.h"

#include "Message.h"
#include "Meta_data.h"
#include "libbyz.h"
#include "th_assert.h"

namespace libbyzea {

int PLevels;
int PSize[MAX_PARTITION_LEVELS];
int PLevelSize[MAX_PARTITION_LEVELS];
int PBlocks[MAX_PARTITION_LEVELS];

namespace partition {
int init(size_t mem_size) {
#ifdef DYNAMIC_PARTITION_TREE
  if (mem_size % Block_size != 0) {
    th_fail("Application state size must be divisble by block size!");
  }

  ssize_t num_blocks = mem_size / Block_size;
  th_assert(num_blocks >= 1, "invalid state");

  ssize_t num_nodes[MAX_PARTITION_LEVELS];
  num_nodes[0] = num_blocks;

  ssize_t num_levels = 1;
  ssize_t children = num_blocks;
  while (children > 1) {
    if (num_levels > MAX_PARTITION_LEVELS) {
      th_fail(
          "invalid configuration: state is too large to be stored in partition "
          "tree");
    }

    children = (children + PChildren - 1) / PChildren;
    num_nodes[num_levels] = children;
    num_levels++;
  }

  PLevels = num_levels;
  PSize[0] = 1;
  PLevelSize[0] = 1;
  PBlocks[num_levels - 1] = 1;
  for (ssize_t i = 1; i < num_levels; i++) {
    PSize[i] = num_nodes[num_levels - i - 1];
    PLevelSize[i] = PLevelSize[i - 1] * PSize[i];
    PBlocks[num_levels - i - 1] = PSize[i] * PBlocks[num_levels - i];
  }
#else
  th_assert(
      PLevels >= 2 && PLevels <= MAX_PARTITION_LEVELS,
      "Invalid configuration: partition tree level must be between 2 and 4");
#endif  // DYNAMIC_PARTITION_TREE

  return 0;
}
}  // namespace partition
}  // namespace libbyzea
