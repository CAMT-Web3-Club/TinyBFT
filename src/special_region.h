#ifndef LIBBYZEA_SPECIAL_REGION_H_
#define LIBBYZEA_SPECIAL_REGION_H_

#include <cstddef>

#include "types.h"

namespace libbyzea {

class View_change;
class View_change_ack;
class New_view;
class New_key;
class Meta_data_d;
class New_view;
class Request;

namespace special_region {

size_t memory_demand();

void init();

New_view *load_new_view(Seqno view);
void store_new_view(New_view *new_view);

View_change *load_view_change(int replica_id);
void store_view_change(View_change *view_change);

View_change_ack *load_view_change_ack(int replica_id, int vc_replica_id);
void store_view_change_ack(View_change_ack *view_change_ack);

New_view *load_new_view(int replica_id);
void store_new_view(New_view *new_view);

Meta_data_d *load_metadata_d(int replica_id);
void store_metadata_d(Meta_data_d *meta_data_d);

New_key *load_new_key();
void store_new_key(New_key *msg);
void free_new_key(New_key *msg);

Request *load_request(int client_id);
void store_request(Request *req);

}  // namespace special_region
}  // namespace libbyzea
#endif  // !LIBBYZEA_SPECIAL_REGION_H_
