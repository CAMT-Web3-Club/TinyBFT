#include "special_region.h"

#include <cstring>
#include <new>

#include "Log_allocator.h"
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

struct NewViewBlock {
  New_view new_view_;
  char msg_[max_new_view_size] __attribute__((aligned(ALIGNMENT)));

  NewViewBlock() : new_view_(reinterpret_cast<New_view_rep *>(msg_)) {}
};

struct ViewChangeBlock {
  View_change view_change_;
  char msg_[max_view_change_size] __attribute__((aligned(ALIGNMENT)));

  ViewChangeBlock() : view_change_(reinterpret_cast<View_change_rep *>(msg_)) {}
  ~ViewChangeBlock() {}
};

struct ViewChangeAckBlock {
  View_change_ack view_change_ack_;
  char msg_[sizeof(View_change_ack_rep) + AUTHENTICATOR_SIZE]
      __attribute__((aligned(ALIGNMENT)));

  ViewChangeAckBlock()
      : view_change_ack_(reinterpret_cast<View_change_ack_rep *>(msg_)) {}
  ~ViewChangeAckBlock() {}
};

struct MetaDataDBlock {
  Meta_data_d meta_data_digest_;
  char msg_[sizeof(Meta_data_d_rep) + AUTHENTICATOR_SIZE]
      __attribute__((aligned(ALIGNMENT)));

  MetaDataDBlock()
      : meta_data_digest_(reinterpret_cast<Meta_data_d_rep *>(msg_)) {}
  ~MetaDataDBlock() {}
};

struct NewKeyBlock {
  New_key new_key_;
  char msg_[max_new_key_size] __attribute__((aligned(ALIGNMENT)));

  NewKeyBlock() : new_key_(reinterpret_cast<New_key_rep *>(msg_)) {}
  ~NewKeyBlock() {}
};

struct RequestBlock {
  Request request_;
  char msg_[max_request_size] __attribute__((aligned(ALIGNMENT)));

  RequestBlock() : request_(reinterpret_cast<Request_rep *>(msg_)) {}
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
  return sizeof(new_views) + sizeof(view_changes) + sizeof(view_change_acks) +
         sizeof(metadata_ds) + sizeof(new_key) + sizeof(requests);
}

void init() { free_new_key(&new_key.new_key_); }

New_view *load_new_view(Seqno view) {
  return &new_views[node->primary(view)].new_view_;
}

void store_new_view(New_view *new_view) {
  th_assert(size_t(new_view->size()) <= max_new_view_size,
            "invalid New_view size");

  std::memcpy(&new_views[node->primary(new_view->view())].msg_,
              new_view->contents(), new_view->size());
}

View_change *load_view_change(int replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &view_changes[replica_id].view_change_;
}

void store_view_change(View_change *view_change) {
  th_assert(view_change->id() <= MAX_NUM_REPLICAS, "Invalid replica id");
  th_assert(size_t(view_change->size()) <= max_view_change_size,
            "invalid View_change size");

  std::memcpy(&view_changes[view_change->id()].msg_, view_change->contents(),
              view_change->size());
}

View_change_ack *load_view_change_ack(int replica_id, int vc_replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &view_change_acks[replica_id][vc_replica_id].view_change_ack_;
}

void store_view_change_ack(View_change_ack *view_change_ack) {
  th_assert(view_change_ack->id() <= MAX_NUM_REPLICAS, "Invalid replica id");
  th_assert(
      size_t(view_change_ack->size()) <= sizeof(view_change_acks[0][0].msg_),
      "invalid View_change_ack size");
  std::memcpy(
      view_change_acks[view_change_ack->id()][view_change_ack->vc_id()].msg_,
      view_change_ack->contents(), view_change_ack->size());
}

Meta_data_d *load_metadata_d(int replica_id) {
  th_assert(replica_id <= MAX_NUM_REPLICAS, "Invalid replica id");
  return &metadata_ds[replica_id].meta_data_digest_;
}

void store_metadata_d(Meta_data_d *meta_data_d) {
  th_assert(meta_data_d->id() <= MAX_NUM_REPLICAS, "Invalid replica id");
  std::memcpy(&metadata_ds[meta_data_d->id()].msg_, meta_data_d->contents(),
              meta_data_d->size());
}

New_key *load_new_key() {
  th_assert(new_key.new_key_.id() != MAGIC, "New_key is freed");
  return &new_key.new_key_;
}

void store_new_key(New_key *msg) {
  th_assert(new_key.new_key_.id() == MAGIC, "New_key already in use");
  th_assert((size_t)msg->size() <= max_new_key_size, "New_key_rep too large");
  std::memcpy(&new_key.msg_, msg->contents(), msg->size());
}

void free_new_key([[maybe_unused]] New_key *msg) {
  th_assert(msg == &new_key.new_key_, "Invalid free pointer");
  th_assert(new_key.new_key_.id() != MAGIC, "Double free");

  auto rep = reinterpret_cast<New_key_rep *>(new_key.msg_);
  rep->id = MAGIC;
}

Request *load_request(int client_id) {
  return &requests[client_id - 4].request_;
}

void store_request(Request *req) {
  int i = req->client_id() - 4;
  th_assert((size_t)req->size() <= sizeof(requests[i].msg_),
            "Request too large");
  std::memcpy(&requests[i].msg_, req->contents(), req->size());
}

}  // namespace special_region
}  // namespace libbyzea
