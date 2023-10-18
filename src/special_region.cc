#include "special_region.h"

#include <cstring>
#include <new>

#include "Meta_data_d.h"
#include "New_key.h"
#include "New_view.h"
#include "Request.h"
#include "View_change.h"
#include "View_change_ack.h"
#include "parameters.h"
#include "th_assert.h"

namespace libbyzea {
namespace special_region {

union NewViewBlock {
  New_view_rep msg;
  char raw[max_new_view_size];
};

union ViewChangeBlock {
  View_change_rep msg;
  char raw[max_view_change_size];

  ViewChangeBlock() { new (&msg) View_change_rep(); }
  ~ViewChangeBlock() {}
};

union ViewChangeAckBlock {
  View_change_ack_rep msg;
  char raw[sizeof(View_change_ack_rep) + AUTHENTICATOR_SIZE];

  ViewChangeAckBlock() { new (&msg) View_change_ack_rep(); }
  ~ViewChangeAckBlock() {}
};

union MetaDataDBlock {
  Meta_data_d_rep msg;
  char raw[sizeof(Meta_data_d_rep) + AUTHENTICATOR_SIZE];

  MetaDataDBlock() { new (&msg) Meta_data_d_rep(); }
  ~MetaDataDBlock() {}
};

union NewKeyBlock {
  New_key_rep msg;
  char raw[1400];
};

union RequestBlock {
  Request_rep msg;
  char raw[max_request_size];

  RequestBlock() { new (&msg) Request_rep(); }
  ~RequestBlock() {}
};

static NewViewBlock new_views[MAX_NUM_REPLICAS];
static ViewChangeBlock view_changes[MAX_NUM_REPLICAS];
static ViewChangeAckBlock view_change_acks[MAX_NUM_REPLICAS][MAX_NUM_REPLICAS];
static MetaDataDBlock metadata_ds[MAX_NUM_REPLICAS];
static NewKeyBlock new_key;
static RequestBlock requests[max_num_clients];

static constexpr int MAGIC = 0xaa5555aa;

size_t memory_demand() {
  return sizeof(view_changes) + sizeof(view_change_acks) + sizeof(metadata_ds) +
         sizeof(new_key);
}

void init() { new_key.msg.id = MAGIC; }

New_view_rep *load_new_view(Seqno view) {
  return &new_views[node->primary(view)].msg;
}

void store_new_view(New_view_rep *new_view) {
  std::memcpy(&new_views[node->primary(new_view->v)].raw, new_view,
              new_view->size);
}

View_change_rep *load_view_change(int replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &view_changes[replica_id].msg;
}

void store_view_change(View_change_rep *view_change) {
  th_assert(view_change->id <= MAX_NUM_REPLICAS, "Invalid replica id");
  std::memcpy(&view_changes[view_change->id].raw, view_change,
              view_change->size);
}

View_change_ack_rep *load_view_change_ack(int replica_id, int vc_replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &view_change_acks[replica_id][vc_replica_id].msg;
}

void store_view_change_ack(View_change_ack_rep *view_change_ack) {
  th_assert(view_change_ack->id <= MAX_NUM_REPLICAS, "Invalid replica id");
  std::memcpy(&view_change_acks[view_change_ack->id][view_change_ack->vcid].raw,
              view_change_ack, view_change_ack->size);
}

Meta_data_d_rep *load_metadata_d(int replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &metadata_ds[replica_id].msg;
}

void store_metadata_d(Meta_data_d_rep *meta_data_d) {
  th_assert(meta_data_d->id <= MAX_NUM_REPLICAS, "Invalid replica id");
  std::memcpy(&metadata_ds[meta_data_d->id].raw, meta_data_d,
              meta_data_d->size);
}

New_key_rep *load_new_key() {
  th_assert(new_key.msg.id != MAGIC, "New_key is freed");
  return &new_key.msg;
}

void store_new_key(New_key_rep *msg) {
  th_assert(new_key.msg.id == MAGIC, "New_key already in use");
  th_assert((size_t)msg->size <= sizeof(NewKeyBlock), "New_key_rep too large");
  std::memcpy(&new_key.raw, msg, msg->size);
}

void free_new_key([[maybe_unused]] New_key_rep *msg) {
  th_assert(msg == &new_key.msg, "Invalid free pointer");
  th_assert(new_key.msg.id != MAGIC, "Double free");

  new_key.msg.id = MAGIC;
}

Request_rep *load_request(int client_id) {
  return &requests[client_id - 4].msg;
}

void store_request(Request_rep *req) {
  th_assert((size_t)req->size <= sizeof(RequestBlock), "Request too large");
  int i = req->cid - 4;
  std::memcpy(&requests[i].raw, req, req->size);
}

}  // namespace special_region
}  // namespace libbyzea
