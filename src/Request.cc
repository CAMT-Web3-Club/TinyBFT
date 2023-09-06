#include "Request.h"

#include <stdlib.h>
#include <strings.h>

#include "MD5.h"
#include "Message_tags.h"
#include "Node.h"
#include "Principal.h"
#include "Statistics.h"
#include "th_assert.h"
#include "types.h"

// extra & 1 = read only

namespace libbyzea {

Request::Request(Request_id r, short rr)
    : Message(Request_tag, Max_message_size) {
  rep().cid = node->id();
  rep().rid = r;
  rep().replier = rr;
  rep().command_size = 0;
  set_size(sizeof(Request_rep));
}

Request *Request::clone() const {
  Request *ret = (Request *)new Message(max_size);
  memcpy(ret->msg, msg, msg->size);
  return ret;
}

char *Request::store_command(int &max_len) {
  int max_auth_size =
      MAX((int)node->principal()->sig_size(), node->auth_size());
  max_len = msize() - sizeof(Request_rep) - max_auth_size;
  return contents() + sizeof(Request_rep);
}

inline void Request::comp_digest(Digest &d) {
  INCR_OP(num_digests);
  START_CC(digest_cycles);
  d.set(reinterpret_cast<const char *>(&(rep().cid)),
        sizeof(int) + sizeof(Request_id) + rep().command_size);

  STOP_CC(digest_cycles);
}

void Request::sign(int act_len) {
  th_assert((unsigned)act_len <=
                msize() - sizeof(Request_rep) - node->principal()->sig_size(),
            "Invalid request size");

  rep().extra |= 2;
  rep().command_size = act_len;
  comp_digest(rep().od);

  int old_size = sizeof(Request_rep) + act_len;
  set_size(old_size + node->principal()->sig_size());
  node->gen_signature(contents(), sizeof(Request_rep), contents() + old_size);
}

Request::Request(Request_rep *contents) : Message(contents) {}

bool Request::verify() {
  const int cid = client_id();
  const size_t old_size = sizeof(Request_rep) + rep().command_size;
  Principal *p = node->i_to_p(cid);
  Digest d;

  comp_digest(d);
  if (p != 0 && d == rep().od) {
    if (size() - old_size >= p->sig_size())
      return p->verify_signature(contents(), sizeof(Request_rep),
                                 contents() + old_size, true);
  }
  return false;
}

bool Request::convert(Message *m1, Request *&m2) {
  if (!m1->has_tag(Request_tag, sizeof(Request_rep))) return false;

  m2 = (Request *)m1;
  m2->trim();
  return true;
}

bool Request::convert(char *m1, unsigned max_len, Request &m2) {
  if (!Message::convert(m1, max_len, Request_tag, sizeof(Request_rep), m2))
    return false;
  return true;
}

}  // namespace libbyzea
