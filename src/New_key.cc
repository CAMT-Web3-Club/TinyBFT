#include "New_key.h"

#include "Message_tags.h"
#include "Node.h"
#include "Principal.h"
#include "th_assert.h"

namespace libbyzea {

New_key::New_key() : Message(New_key_tag, Max_message_size) {
  unsigned k[Nonce_size_u];

  rep().rid = node->new_rid();
  rep().padding = 0;
  node->principal()->set_out_key(k, rep().rid);
  rep().id = node->id();

  // Get new keys and encrypt them
  Principal *p;
  char *dst = contents() + sizeof(New_key_rep);
  size_t dst_len = Max_message_size - sizeof(New_key_rep);
  for (int i = 0; i < node->n(); i++) {
    // Skip myself.
    if (i == node->id()) continue;

    random_nonce(k);
    p = node->i_to_p(i);
    p->set_in_key(k);
    unsigned ssize = p->encrypt((char *)k, Nonce_size, dst, dst_len);
    th_assert(ssize != 0U, "Message is too small");
    dst += ssize;
    dst_len -= ssize;
  }
  // set my size to reflect the amount of space in use
  set_size(Max_message_size - dst_len);

  // Compute signature and update size.
  p = node->principal();
  int old_size = size();
  th_assert(dst_len >= p->sig_size(), "Message is too small");
  set_size(size() + p->sig_size());
  node->gen_signature(contents(), old_size, contents() + old_size);
}

#ifdef STATIC_LOG_ALLOCATOR
New_key::~New_key() {
  if (msg == nullptr) {
    return;
  }
  if (!in_scratch_) {
    special_region::free_new_key(&(rep()));
  }
}
#endif

bool New_key::verify() {
  // If bad principal or old message discard.
  Principal *p = node->i_to_p(id());
  if (p == 0 || p->last_tstamp() >= rep().rid) {
    return false;
  }

  // Check signature
  if ((unsigned)size() < p->sig_size()) {
    return false;
  }
  size_t msg_len = size() - p->sig_size();
  char *signature = contents() + msg_len;
  if (!p->verify_signature(contents(), msg_len, signature)) {
    return false;
  }

  char *dst = contents() + sizeof(New_key_rep);
  size_t dst_len = size() - sizeof(New_key_rep);
  unsigned k[Nonce_size_u];

  for (int i = 0; i < node->n(); i++) {
    // Skip principal that sent message
    if (i == id()) continue;

    int ssize = cipher_size(dst, dst_len);
    if (ssize == 0) return false;

    if (i == node->id()) {
      // found my key
      int ksize = node->decrypt(dst, dst_len, (char *)k, Nonce_size);
      if (ksize != Nonce_size) return false;
    }

    dst += ssize;
    dst_len -= ssize;
  }

  p->set_out_key(k, rep().rid);

  return true;
}

bool New_key::convert(Message *m1, New_key *&m2) {
  if (!m1->has_tag(New_key_tag, sizeof(New_key_rep))) return false;

  m1->trim();
  m2 = (New_key *)m1;
  return true;
}

}  // namespace libbyzea
