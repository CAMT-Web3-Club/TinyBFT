#ifndef LIBBYZEA_AGREEMENT_REGION_H_
#define LIBBYZEA_AGREEMENT_REGION_H_

#include <cstddef>

#include "parameters.h"
#include "types.h"

namespace libbyzea {

class Pre_prepare;
class Prepare;
class Commit;

namespace agreement_region {

size_t memory_demand();

Pre_prepare *load_pre_prepare(Seqno n);
void store_pre_prepare(Pre_prepare *Pre_prepare);

Prepare *load_prepare(Seqno n, size_t i);
void store_prepare(Prepare *Pre_prepare, size_t i);

Commit *load_commit(Seqno n, size_t i);
void store_commit(Commit *Commit, size_t i);

void truncate(Seqno new_head);
bool within_range(Seqno seqno);
}  // namespace agreement_region
}  // namespace libbyzea
#endif  // !LIBBYZEA_AGREEMENT_REGION_H_
