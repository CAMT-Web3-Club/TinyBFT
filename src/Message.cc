#include "Message.h"

#include <stdlib.h>

#include "Node.h"
#include "scratch_allocator.h"
#include "th_assert.h"

namespace libbyzea {

Log_allocator *Message::a = nullptr;

Message::Message(unsigned sz)
#ifdef STATIC_LOG_ALLOCATOR
    : msg(0), max_size(ALIGNED_SIZE(sz)), in_scratch_(false) {
#else
    : msg(0), max_size(ALIGNED_SIZE(sz)) {
#endif
  if (sz != 0) {
#ifndef STATIC_LOG_ALLOCATOR
    msg = (Message_rep *)a->malloc(max_size);
#else
    msg = (Message_rep *)scratch_allocator::malloc(max_size);
    in_scratch_ = true;
#endif

    th_assert(ALIGNED(msg), "Improperly aligned pointer");
    msg->tag = -1;
    msg->size = 0;
    msg->extra = 0;
  }
}

Message::Message(int t, unsigned sz) {
  max_size = ALIGNED_SIZE(sz);
#ifndef STATIC_LOG_ALLOCATOR
  msg = (Message_rep *)a->malloc(max_size);
#else
  msg = (Message_rep *)scratch_allocator::malloc(max_size);
  in_scratch_ = true;
#endif

  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  msg->tag = t;
  msg->size = max_size;
  msg->extra = 0;
}

Message::Message(Message_rep *cont) {
  th_assert(ALIGNED(cont), "Improperly aligned pointer");

#ifdef STATIC_LOG_ALLOCATOR
  in_scratch_ = scratch_allocator::is_in_scratch(cont);
#endif
  msg = cont;
  max_size = -1;  // To prevent contents from being deallocated or trimmed
}

#ifdef STATIC_LOG_ALLOCATOR
Message::Message(int t, Message_rep *contents) {
  th_assert(ALIGNED(contents), "Improperly aligned pointer");
  msg = contents;
  max_size = -1;  // To prevent contents from being deallocated or trimmed

  msg->tag = t;
}
#endif

Message::~Message() {
#ifndef STATIC_LOG_ALLOCATOR
  if (max_size > 0) {
    a->free((char *)msg, max_size);
  }
#else
  if (max_size > 0 && in_scratch_) {
    scratch_allocator::free(msg, size_t(max_size));
  }
#endif
}

void Message::trim() {
#ifndef STATIC_LOG_ALLOCATOR
  if (max_size > 0 && a->realloc((char *)msg, max_size, msg->size)) {
    max_size = msg->size;
  }
#else
  if (max_size > 0 && in_scratch_) {
    scratch_allocator::realloc(msg, msg->size);
  }
#endif
}

void Message::set_size(int size) {
  th_assert(msg && ALIGNED(msg), "Invalid state");
  th_assert(max_size < 0 || ALIGNED_SIZE(size) <= max_size, "Invalid state");
  int aligned = ALIGNED_SIZE(size);
  for (int i = size; i < aligned; i++) ((char *)msg)[i] = 0;
  msg->size = aligned;
}

bool Message::convert(char *src, unsigned len, int t, int sz, Message &m) {
  // First check if src is large enough to hold a Message_rep
  if (len < sizeof(Message_rep)) return false;

  // Check alignment.
  if (!ALIGNED(src)) return false;

  // Next check tag and message size
  Message ret((Message_rep *)src);
  if (!ret.has_tag(t, sz)) return false;

  m = ret;
  return true;
}

bool Message::encode(FILE *o) {
  int csize = size();

  size_t sz = fwrite(&max_size, sizeof(int), 1, o);
  sz += fwrite(&csize, sizeof(int), 1, o);
  sz += fwrite(msg, 1, csize, o);

  return sz == 2U + csize;
}

bool Message::decode(FILE *i) {
  delete msg;

  size_t sz = fread(&max_size, sizeof(int), 1, i);
  msg = (Message_rep *)a->malloc(max_size);

  int csize;
  sz += fread(&csize, sizeof(int), 1, i);

  if (msg == 0 || csize < 0 || csize > max_size) return false;

  sz += fread(msg, 1, csize, i);
  return sz == 2U + csize;
}

void Message::init() {
#ifndef STATIC_LOG_ALLOCATOR
  a = new Log_allocator();
#endif
}

const char *Message::stag() {
  static const char *string_tags[] = {
      "Free_message", "Request",     "Reply",           "Pre_prepare",
      "Prepare",      "Commit",      "Checkpoint",      "Status",
      "View_change",  "New_view",    "View_change_ack", "New_key",
      "Meta_data",    "Meta_data_d", "Data_tag",        "Fetch",
      "Query_stable", "Reply_stable"};
  return string_tags[tag()];
}

}  // namespace libbyzea
