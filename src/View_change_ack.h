#ifndef _View_change_ack_h
#define _View_change_ack_h 1

#include "Digest.h"
#include "Message.h"
#include "scratch_allocator.h"
#include "special_region.h"
#include "types.h"

namespace libbyzea {

class Principal;

//
// View_change_ack messages have the following format:
//
struct View_change_ack_rep : public Message_rep {
  View v;
  int id;
  int vcid;
  Digest vcd;
  // Followed by a MAC for the intended recipient
};

class View_change_ack : public Message {
  //
  // View_change_ack messages
  //
 public:
  View_change_ack(View_change_ack_rep* msg) : Message(msg) {}

  View_change_ack(View v, int id, int vcid, Digest const& vcd);
  // Effects: Creates a new authenticated View_change_ack message for
  // replica "id" stating that replica "vcid" sent out a view-change
  // message for view "v" with digest "vcd". The MAC is for the primary
  // of "v".

  void re_authenticate(Principal* p = 0);
  // Effects: Recomputes the MAC in the message using the
  // most recent keys. If "p" is not null, computes a MAC for "p"
  // rather than for the primary of "view()".

  View view() const;
  // Effects: Fetches the view number from the message.

  int id() const;
  // Effects: Fetches the identifier of the replica from the message.

  int vc_id() const;
  // Effects: Fetches the identifier of the replica whose view-change
  // message is being acked.

  Digest& vc_digest() const;
  // Effects: Fetches the digest of the view-change message that is
  // being acked.

  bool match(const View_change_ack* p) const;
  // Effects: Returns true iff "p" and "this" match.

  bool verify();
  // Effects: Verifies if the message is signed by the replica rep().id.

  static bool convert(Message* m1, View_change_ack*& m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "View_change_ack" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false.

#ifdef STATIC_LOG_ALLOCATOR
  View_change_ack* persist();
#endif

 private:
  View_change_ack_rep& rep() const;
  // Effects: Casts contents to a View_change_ack_rep&
};

inline View_change_ack_rep& View_change_ack::rep() const {
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((View_change_ack_rep*)msg);
}

inline View View_change_ack::view() const { return rep().v; }

inline int View_change_ack::id() const { return rep().id; }

inline int View_change_ack::vc_id() const { return rep().vcid; }

inline Digest& View_change_ack::vc_digest() const { return rep().vcd; }

inline bool View_change_ack::match(const View_change_ack* p) const {
  th_assert(view() == p->view(), "Invalid argument");
  return vc_id() == p->vc_id() && vc_digest() == p->vc_digest();
}

#ifdef STATIC_LOG_ALLOCATOR
inline View_change_ack* View_change_ack::persist() {
  th_assert(in_scratch_, "Message is already persisted in another certificate");

  int replica_id = id();
  int vc_replica_id = vc_id();
  special_region::store_view_change_ack(this);
  return special_region::load_view_change_ack(replica_id, vc_replica_id);
}
#endif

}  // namespace libbyzea

#endif  // _View_change_ack_h
