#ifndef LIBBYZEA_AGREEMENT_REGION_H_
#define LIBBYZEA_AGREEMENT_REGION_H_

#include <cstddef>

#include "parameters.h"
#include "types.h"

namespace libbyzea {

class Pre_prepare_rep;
class Prepare_rep;
class Commit_rep;

namespace agreement_region {

size_t memory_demand();

Pre_prepare_rep *load_pre_prepare(Seqno n);
void store_pre_prepare(Pre_prepare_rep *Pre_prepare);

Prepare_rep *load_prepare(Seqno n, size_t i);
void store_prepare(Prepare_rep *Pre_prepare, size_t i);

Commit_rep *load_commit(Seqno n, size_t i);
void store_commit(Commit_rep *Commit, size_t i);

void truncate(Seqno new_head);
bool within_range(Seqno seqno);
}  // namespace agreement_region
}  // namespace libbyzea
#endif  // !LIBBYZEA_AGREEMENT_REGION_H_
