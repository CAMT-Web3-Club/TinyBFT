#ifndef LIBBYZEA_CHECKPOINT_REGION_H_
#define LIBBYZEA_CHECKPOINT_REGION_H_

#include "parameters.h"
#include "types.h"

namespace libbyzea {

class Checkpoint_rep;

namespace checkpoint_region {

size_t memory_demand();

Seqno max_seqno();

bool within_range(Seqno seqno);

Checkpoint_rep *load_checkpoint(Seqno seqno, size_t i);
void store_checkpoint(Checkpoint_rep *checkpoint, size_t i);

void truncate(Seqno seqno);

int certificate_index(Seqno seqno);

}  // namespace checkpoint_region
}  // namespace libbyzea

#endif  // !LIBBYZEA_CHECKPOINT_REGION_H_
