#ifndef _Pre_Prepare_h
#define _Pre_Prepare_h 1

#include "Digest.h"
#include "Message.h"
#include "Prepare.h"
#include "agreement_region.h"
#include "scratch_allocator.h"
#include "types.h"

namespace libbyzea {

class Principal;
class Req_queue;
class Request;

//
// Pre_prepare messages have the following format:
//
struct Pre_prepare_rep : public Message_rep {
  View view;
  Seqno seqno;
  Digest digest;       // digest of request set concatenated with
                       // non-deterministic choices
  int rset_size;       // size in bytes of request set
  short non_det_size;  // size in bytes of non-deterministic choices

  // Followed by "rset_size" bytes of the request set "non_det_size" bytes of
  // non-deterministic choices, and a variable length signature in the above
  // order.
};

class Prepare;

class Pre_prepare : public Message {
  //
  // Pre_prepare messages
  //
 public:
  Pre_prepare() : Message() {}

  Pre_prepare(Pre_prepare_rep* msg) : Message(msg) {}

  Pre_prepare(View v, Seqno s, Req_queue& reqs);
  // Effects: Creates a new signed Pre_prepare message with view
  // number "v", sequence number "s", the requests in "reqs" (up to a
  // maximum size) and appropriate non-deterministic choices.  It
  // removes the elements of "reqs" that are included in the message
  // from "reqs" and deletes them.

  char* choices(int& len);
  // Effects: Returns a buffer that can be filled with non-deterministic choices

  Pre_prepare* clone(View v) const;
  // Effects: Creates a new object with the same state as this but view v.

  void re_authenticate(Principal* p = 0);
  // Effects: Recomputes the authenticator in the message using the most
  // recent keys. If "p" is not null, may only update "p"'s
  // entry.

  View view() const;
  // Effects: Fetches the view number from the message.

  Seqno seqno() const;
  // Effects: Fetches the sequence number from the message.

  int id() const;
  // Effects: Returns the identifier of the primary for view() (which is
  // the replica that sent the message if the message is correct.)

  bool match(const Prepare* p) const;
  // Effects: Returns true iff "p" and "this" match.

  Digest& digest() const;
  // Effects: Fetches the digest from the message.

  class Requests_iter {
    // An iterator for yielding the Requests in a Pre_prepare message.
    // Requires: A Pre_prepare message cannot be modified while it is
    // being iterated on.
   public:
    Requests_iter(Pre_prepare* m);
    // Requires: Pre_prepare is known to be valid
    // Effects: Return an iterator for the requests in "m"

    bool get(Request& req);
    // Effects: Updates "req" to "point" to the next request in the
    // Pre_prepare message and returns true. If there are no more
    // requests, it returns false.

   private:
    Pre_prepare* msg;
    char* next_req;
  };
  friend class Requests_iter;

  static const int NAC = 1;
  static const int NRC = 2;
  bool verify(int mode = 0);
  // Effects: If "mode == 0", verifies if the message is authenticated
  // by the replica "id()", if the digest is correct, and if the
  // requests are authentic. If "mode == NAC", it performs all checks
  // except that it does not check if the message is authenticated by
  // the replica "id()". If "mode == NRC", it performs all checks
  // except that it does not verify the authenticity of the requests.

  bool check_digest();
  // Effects: Verifies if the digest is correct.

#ifdef STATIC_LOG_ALLOCATOR
  Pre_prepare* persist();
#endif

  static bool convert(Message* m1, Pre_prepare*& m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "Pre_prepare" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false.

 private:
  Pre_prepare_rep& rep() const;
  // Effects: Casts contents to a Pre_prepare_rep&

  char* requests();
  // Effects: Returns a pointer to the first request contents.

  char* non_det_choices();
  // Effects: Returns a pointer to the buffer with non-deterministic
  // choices.
};

inline Pre_prepare_rep& Pre_prepare::rep() const {
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((Pre_prepare_rep*)msg);
}

inline char* Pre_prepare::requests() {
  char* ret = contents() + sizeof(Pre_prepare_rep);
  th_assert(ALIGNED(ret), "Improperly aligned pointer");
  return ret;
}

inline char* Pre_prepare::non_det_choices() {
  char* ret = requests() + rep().rset_size;
  th_assert(ALIGNED(ret), "Improperly aligned pointer");
  return ret;
}

inline char* Pre_prepare::choices(int& len) {
  len = rep().non_det_size;
  return non_det_choices();
}

inline View Pre_prepare::view() const { return rep().view; }

inline Seqno Pre_prepare::seqno() const { return rep().seqno; }

inline bool Pre_prepare::match(const Prepare* p) const {
  th_assert(view() == p->view() && seqno() == p->seqno(), "Invalid argument");
  return digest() == p->digest();
}

inline Digest& Pre_prepare::digest() const { return rep().digest; }

#ifdef STATIC_LOG_ALLOCATOR
inline Pre_prepare* Pre_prepare::persist() {
  Seqno sn = seqno();
  agreement_region::store_pre_prepare(this);
  return agreement_region::load_pre_prepare(sn);
}
#endif

}  // namespace libbyzea

#endif  // _Pre_prepare_h
