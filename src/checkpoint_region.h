#ifndef LIBBYZEA_CHECKPOINT_REGION_H_
#define LIBBYZEA_CHECKPOINT_REGION_H_

#include "parameters.h"
#include "types.h"

namespace libbyzea {

class Checkpoint;

namespace checkpoint_region {

size_t memory_demand();

Seqno max_seqno();

bool within_range(Seqno seqno);

Checkpoint *load_checkpoint(Seqno seqno, size_t i);
void store_checkpoint(Checkpoint *checkpoint, size_t i);

Checkpoint *load_above_window(size_t replica_id);
void store_above_window(Checkpoint *checkpoint);

void truncate(Seqno seqno);

int certificate_index(Seqno seqno);

}  // namespace checkpoint_region
}  // namespace libbyzea

#endif  // !LIBBYZEA_CHECKPOINT_REGION_H_
