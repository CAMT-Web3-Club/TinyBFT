#include "special_region.h"

#include <cstring>
#include <new>

#include "Log_allocator.h"
#include "Message_tags.h"
#include "Meta_data_d.h"
#include "New_key.h"
#include "New_view.h"
#include "Rep_info.h"
#include "Reply.h"
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

  NewKeyBlock() : new_key_(reinterpret_cast<New_key_rep *>(msg_)) {
    auto nkr = reinterpret_cast<New_key_rep *>(msg_);
    nkr->tag = New_key_tag;
  }
  ~NewKeyBlock() {}
};

struct RequestBlock {
  Request request_;
  char msg_[max_request_size] __attribute__((aligned(ALIGNMENT)));

  RequestBlock() : request_(reinterpret_cast<Request_rep *>(msg_)) {}
  ~RequestBlock() {}
};

struct ReplyBlock {
  Reply reply_;
  char msg_[Rep_info::Max_rep_size];

  ReplyBlock() : reply_(reinterpret_cast<Reply_rep *>(msg_)) {}
  ~ReplyBlock() {}
};

static NewViewBlock new_views[MAX_NUM_REPLICAS];
static ViewChangeBlock view_changes[MAX_NUM_REPLICAS];
static ViewChangeAckBlock view_change_acks[MAX_NUM_REPLICAS][MAX_NUM_REPLICAS];
static MetaDataDBlock metadata_ds[MAX_NUM_REPLICAS];
static NewKeyBlock new_key;
static RequestBlock requests[max_num_clients + Max_num_replicas];
static ReplyBlock replies[F + 1];

size_t memory_demand_checkpoints() { return sizeof(metadata_ds); }

size_t memory_demand_view() {
  return sizeof(new_views) + sizeof(view_changes) + sizeof(view_change_acks);
}

size_t memory_demand_crypto() { return sizeof(new_key); }

size_t memory_demand_requests() { return sizeof(requests); }

size_t memory_demand() {
  return sizeof(new_views) + sizeof(view_changes) + sizeof(view_change_acks) +
         sizeof(metadata_ds) + sizeof(new_key) + sizeof(requests);
}

New_view *new_new_view(View v) {
  auto &b = new_views[node->id()];
  auto nvr = reinterpret_cast<New_view_rep *>(b.msg_);
  return new (&b.new_view_) New_view(nvr, v);
}

New_view *load_new_view(Seqno view) {
  return &new_views[node->primary(view)].new_view_;
}

void store_new_view(New_view *new_view) {
  th_assert(size_t(new_view->size()) <= max_new_view_size,
            "invalid New_view size");

  fprintf(stderr, "%d %d\n", new_view->size(), max_new_view_size);
  std::memcpy(&new_views[node->primary(new_view->view())].msg_,
              new_view->contents(), new_view->size());
}

View_change *new_view_change(View v, Seqno ls, int replica_id) {
  auto &b = view_changes[replica_id];
  auto vcr = reinterpret_cast<View_change_rep *>(b.msg_);
  return new (&b.view_change_) View_change(vcr, v, ls, replica_id);
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

View_change_ack *new_view_change_ack(View v, int vc_replica_id,
                                     Digest const &digest) {
  th_assert(replica_id == node->id(), "invalid state");
  auto &b = view_change_acks[node->id()][vc_replica_id];
  auto vcar = reinterpret_cast<View_change_ack_rep *>(b.msg_);

  return new (&b.view_change_ack_)
      View_change_ack(vcar, v, vc_replica_id, digest);
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

New_key *load_new_key() { return &new_key.new_key_; }

Request *new_request(Request_id rid, int rr) {
  auto &b = requests[node->id()];
  return new (&b.request_)
      Request(reinterpret_cast<Request_rep *>(b.msg_), rid, rr);
}
Request *load_request(int client_id) { return &requests[client_id].request_; }

void store_request(Request *req) {
  int i = req->client_id();
  th_assert((size_t)req->size() <= sizeof(requests[i].msg_),
            "Request too large");
  std::memcpy(&requests[i].msg_, req->contents(), req->size());
}

Reply *load_reply(size_t slot) { return &replies[slot].reply_; }

void store_reply(Reply *reply, size_t slot) {
  std::memcpy(&replies[slot].msg_, reply->contents(), reply->size());
}

}  // namespace special_region
}  // namespace libbyzea
