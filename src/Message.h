#ifndef _Message_h
#define _Message_h 1

#include <stddef.h>
#include <stdio.h>

#include <cstdlib>

#include "Digest.h"
#include "Log_allocator.h"
#include "Message_tags.h"
#include "mem_statistics.h"
#include "scratch_allocator.h"
#include "th_assert.h"
#include "types.h"

#ifndef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE 9000
#endif  // MAX_MESSAGE_SIZE

namespace libbyzea {

// Maximum message size. Must verify ALIGNED_SIZE.
const size_t Max_message_size = MAX_MESSAGE_SIZE;

constexpr int F = MAX_NUM_REPLICAS / 3;

#ifdef PKEY
// Assume a maximum RSA key-size of 2048 bit
constexpr unsigned AUTHENTICATOR_SIZE = 256;
#else
constexpr unsigned AUTHENTICATOR_SIZE = Digest::SIZE * (MAX_NUM_REPLICAS - 1);
#endif

//
// All messages have the following format:
//
struct Message_rep {
  short tag;
  short extra;  // May be used to store extra information.
  int size;     // Must be a multiple of 8 bytes to ensure proper
                // alignment of embedded messages.

  // Followed by char payload[size-sizeof(Message_rep)];
};

class Message {
  //
  // Generic messages
  //
 public:
  /**
   * Effects: Creates an untagged Message object that can hold up
   * to "sz" bytes and holds zero bytes. Useful to create message
   * buffers to receive messages from the network.
   */
  Message(unsigned sz = 0);

  /**
   * Effects: Deallocates all storage associated with this message.
   */
  ~Message();

  /** Effects: Deallocates surplus storage. */
  void trim();

  /** Effects: Return a byte string with the message contents.
   * TODO: should be protected here because of request iterator in
   * Pre_prepare.cc
   */
  char *contents();

  /** Effects: Fetches the message size. */
  int size() const;

  int tag() const;
  // Effects: Fetches the message tag.

  bool has_tag(int t, int sz) const;
  // Effects: If message has tag "t", its size is greater than "sz",
  // its size less than or equal to "max_size", and its size is a
  // multiple of ALIGNMENT, returns true.  Otherwise, returns false.

  /**
   * Effects: Returns any view associated with the message or 0.
   */
  View view() const;

  /**
   * Effects: Messages may be full or empty. Empty messages are just
   * digests of full messages.
   */
  bool full() const;

  /** Message-specific heap management operators. */
  void *operator new(size_t s);
  void *operator new(size_t s, void *p);
  void operator delete(void *x, size_t s);

  /**
   * Effects: Should be called once to initialize the memory allocator.
   * Before any message is allocated.
   */
  static void init();

  /** Effects: Returns a string with tag name */
  const char *stag();

  /**
   * Effects: Encodes object state from stream. Return
   * true if successful and false otherwise.
   */
  bool encode(FILE *o);

  /**
   * Effects: Decodes object state from stream. Return
   * true if successful and false otherwise.
   */
  bool decode(FILE *i);

  /**
   * Effects: Prints debug information for memory allocator.
   */
  static void debug_alloc() {
#ifndef STATIC_LOG_ALLOCATOR
    if (a) a->debug_print();
#endif
  }

 protected:
  Message(int t, unsigned sz);
  // Effects: Creates a message with tag "t" that can hold up to "sz"
  // bytes. Useful to create messages to send to the network.

  Message(Message_rep *contents);
  // Requires: "contents" contains a valid Message_rep.
  // Effects: Creates a message from "contents". No copy is made of
  // "contents" and the storage associated with "contents" is not
  // deallocated if the message is later deleted. Useful to create
  // messages from reps contained in other messages.

  void set_size(int size);
  // Effects: Sets message size to the smallest multiple of 8 bytes
  // greater than equal to "size" and pads the message with zeros
  // between "size" and the new message size. Important to ensure
  // proper alignment of embedded messages.

  int msize() const;
  // Effects: Fetches the maximum number of bytes that can be stored in
  // this message.

  static bool convert(char *src, unsigned len, int t, int sz, Message &m);
  // Requires: convert can safely read up to "len" bytes starting at
  // "src" Effects: If "src" is a Message_rep for which "has_tag(t,
  // sz)" returns true and sets m to have contents "src". Otherwise,
  // it returns false.  No copy is made of "src" and the storage
  // associated with "contents" is not deallocated if "m" is later
  // deleted.

  friend class Node;
  friend class Pre_prepare;

  Message_rep *msg;  // Pointer to the contents of the message.
  int max_size;      // Maximum number of bytes that can be stored in "msg"
                     // or "-1" if this instance is not responsible for
                     // deallocating the storage in msg.
                     // Invariant: max_size <= 0 || 0 < msg->size <= max_size
  bool in_scratch_;

 private:
  //
  // Message-specific memory management
  //
  static Log_allocator *a;
};

// Methods inlined for speed

inline int Message::size() const { return msg->size; }

inline int Message::tag() const { return msg->tag; }

inline bool Message::has_tag(int t, int sz) const {
  if (max_size >= 0 && msg->size > max_size) return false;

  if (!msg || msg->tag != t || msg->size < sz || !ALIGNED(msg->size))
    return false;
  return true;
}

inline View Message::view() const { return 0; }

inline bool Message::full() const { return true; }

inline void *Message::operator new(size_t s) {
#ifndef STATIC_LOG_ALLOCATOR
  void *ret = (void *)a->malloc(ALIGNED_SIZE(s));
#else
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_LOG_ALLOCATOR);
  void *ret = malloc(ALIGNED_SIZE(s));
  MEMSTATS_RESTORE_MEM_TYPE();
#endif
  th_assert(ret != 0, "Ran out of memory\n");
  return ret;
}

inline void *Message::operator new([[maybe_unused]] size_t s, void *p) {
  return p;
}

inline void Message::operator delete(void *x, [[maybe_unused]] size_t s) {
  if (x == nullptr) {
    return;
  }

#ifndef STATIC_LOG_ALLOCATOR
  a->free((char *)x, ALIGNED_SIZE(s));
#else
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_LOG_ALLOCATOR);
  free(x);
  MEMSTATS_RESTORE_MEM_TYPE();
#endif
}

inline int Message::msize() const {
  return (max_size >= 0) ? max_size : msg->size;
}

inline char *Message::contents() { return (char *)msg; }

}  // namespace libbyzea

#endif  //_Message_h
