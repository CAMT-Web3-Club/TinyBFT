#ifndef LIBBYZEA_SPECIAL_REGION_H_
#define LIBBYZEA_SPECIAL_REGION_H_

#include <cstddef>

#include "types.h"

namespace libbyzea {

class View_change_rep;
class View_change_ack_rep;
class New_view_rep;
class New_key_rep;
class Meta_data_d_rep;
class New_view_rep;

namespace special_region {

size_t memory_demand();

void init();

New_view_rep *load_new_view(Seqno view);
void store_new_view(New_view_rep *new_view);

View_change_rep *load_view_change(int replica_id);
void store_view_change(View_change_rep *view_change);

View_change_ack_rep *load_view_change_ack(int replica_id, int vc_replica_id);
void store_view_change_ack(View_change_ack_rep *view_change_ack);

New_view_rep *load_new_view(int replica_id);
void store_new_view(New_view_rep *new_view);

Meta_data_d_rep *load_metadata_d(int replica_id);
void store_metadata_d(Meta_data_d_rep *meta_data_d);

New_key_rep *load_new_key();
void store_new_key(New_key_rep *msg);
void free_new_key(New_key_rep *msg);

}  // namespace special_region
}  // namespace libbyzea
#endif  // !LIBBYZEA_SPECIAL_REGION_H_
