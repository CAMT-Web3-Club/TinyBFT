#include "Node.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

#include "ITimer.h"
#include "Message.h"
#include "New_key.h"
#include "Principal.h"
#include "Time.h"
#include "parameters.h"
#include "th_assert.h"

#define NO_IP_MULTICAST

#ifndef NDEBUG
#define NDEBUG
#endif

// Pointer to global node instance.
Node *node = 0;

// Enable statistics
#include "Statistics.h"

static const char *DRBG_PERSONALIZATION_STRING =
    static_cast<const char *>("libbyz");

Node::Node(FILE *config_file, const std::string &private_key_file,
           short req_port) {
  node = this;

#ifdef NO_IP_MULTICAST
  fprintf(stderr, "WARNING: disabled multicast\n");
#endif

  // Intialize random number generator
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
  int err = mbedtls_ctr_drbg_seed(
      &ctr_drbg_ctx, mbedtls_entropy_func, &entropy,
      reinterpret_cast<const unsigned char *>(DRBG_PERSONALIZATION_STRING),
      std::strlen(DRBG_PERSONALIZATION_STRING));
  if (err) {
    th_fail("Failed to seed random number generator");
  }

  // Compute clock frequency.
  init_clock_mhz();

  // Read private key file:
  // TODO: this file should be encrypted under some passphrase and user
  // should be prompted for that passphrase.
  priv_key = new libbyz::RsaPrivateKey(private_key_file, &ctr_drbg_ctx);

  // Read public configuration file:
  // TODO: this should be more robust
  fscanf(config_file, "%256s\n", service_name);

  // read max_faulty and compute derived variables
  fscanf(config_file, "%d\n", &max_faulty);
  num_replicas = 3 * max_faulty + 1;
  if (num_replicas > Max_num_replicas) th_fail("Invalid number of replicas");
  threshold = num_replicas - max_faulty;

  // Read authentication timeout
  int at;
  fscanf(config_file, "%d\n", &at);

  // read in all the principals
  char addr_buff[100];
  char public_keyfile[1024];
  short port;

  fscanf(config_file, "%d\n", &num_principals);
  //  if (num_replicas > num_principals)
  //  th_fail("Invalid argument");

  // read in group principal's address
  fscanf(config_file, "%256s %hd\n", addr_buff, &port);
  Addr a;
  bzero((char *)&a, sizeof(a));
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = inet_addr(addr_buff);
  a.sin_port = htons(port);
  group = new Principal(num_principals + 1, a, &ctr_drbg_ctx);

  // read in remaining principals' addresses and figure out my principal
  char host_name[MAXHOSTNAMELEN + 1];
  if (gethostname(host_name, MAXHOSTNAMELEN)) {
    perror("Unable to get hostname");
    exit(1);
  }
  struct hostent *hent = gethostbyname(host_name);
  if (hent == 0) th_fail("Could not get hostent");
  struct in_addr my_address = *((in_addr *)hent->h_addr_list[0]);
  node_id = -1;

  principals = (Principal **)malloc(num_principals * sizeof(Principal *));
  for (int i = 0; i < num_principals; i++) {
    fscanf(config_file, "%256s %32s %hd %1024s \n", host_name, addr_buff, &port,
           public_keyfile);
    a.sin_addr.s_addr = inet_addr(addr_buff);
    a.sin_port = htons(port);
    principals[i] = new Principal(i, a, &ctr_drbg_ctx, public_keyfile);
    if (my_address.s_addr == a.sin_addr.s_addr && node_id == -1 &&
        (req_port == 0 || req_port == port)) {
      node_id = i;
    }
  }

  if (node_id < 0) th_fail("Could not find my principal");

  // Initialize current view number and primary.
  v = 0;
  cur_primary = 0;

  // Initialize memory allocator for messages.
  Message::init();

  // Initialize socket.
  sock = socket(AF_INET, SOCK_DGRAM, 0);

  // name the socket
  Addr tmp;
  tmp.sin_family = AF_INET;
  tmp.sin_addr.s_addr = htonl(INADDR_ANY);
  tmp.sin_port = principals[node_id]->address()->sin_port;
  int error = bind(sock, (struct sockaddr *)&tmp, sizeof(Addr));
  if (error < 0) {
    perror("Unable to name socket");
    exit(1);
  }

#define WANMCAST 0
#ifdef WANMCAST
  // Set TTL larger than 1 to enable multicast across routers.
  u_char i = 20;
  error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&i, sizeof(i));
  if (error < 0) {
    perror("unable to change TTL value");
    exit(1);
  }
#endif

  //#define NO_UDP_CHECKSUM
#ifdef NO_UDP_CHECKSUM
  int no_check = 1;
  error = setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, (char *)&no_check,
                     sizeof(no_check));
  if (error < 0) {
    perror("unable to turn of UDP checksumming");
    exit(1);
  }
#endif  // NO_UDP_CHECKSUM

#define ASYNC_SOCK
#ifdef ASYNC_SOCK
  error = fcntl(sock, F_SETFL, O_NONBLOCK);

  if (error < 0) {
    perror("unable to set socket to asynchronous mode");
    exit(1);
  }
#endif  // ASYNC_SOCK

  // Sleep for more than a second to ensure strictly increasing
  // timestamps.
  sleep(2);

  // Compute new timestamp for cur_rid
  new_tstamp();

  last_new_key = 0;
  atimer = new ITimer(at, atimer_handler);
}

Node::~Node() {
  for (int i = 0; i < num_principals; i++) delete principals[i];

  free(principals);
  delete group;
  delete priv_key;
}

void Node::send(Message *m, int i) {
  th_assert(i == All_replicas || (i >= 0 && i < num_principals),
            "Invalid argument");

#ifdef NO_IP_MULTICAST
  if (i == All_replicas) {
    for (int x = 0; x < num_replicas; x++) send(m, x);
    return;
  }
#endif

  const Addr *to =
      (i == All_replicas) ? group->address() : principals[i]->address();

  int error = 0;
  int size = m->size();
  while (error < size) {
    INCR_OP(num_sendto);
    INCR_CNT(bytes_out, size);
    START_CC(sendto_cycles);

    error = sendto(sock, m->contents(), size, 0, (struct sockaddr *)to,
                   sizeof(Addr));

    STOP_CC(sendto_cycles);
#ifndef NDEBUG
    if (error < 0 && error != EAGAIN) perror("Node::send: sendto");
#endif
  }
}

bool Node::has_messages(long to) {
  START_CC(handle_timeouts_cycles);
  ITimer::handle_timeouts();
  STOP_CC(handle_timeouts_cycles);

  // Timeout period for select. It puts a lower bound on the timeout
  // granularity for other timers.
  START_CC(select_cycles);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = to;
  fd_set fdset;
  FD_ZERO(&fdset);
  FD_SET(sock, &fdset);
  int ret = select(sock + 1, &fdset, 0, 0, &timeout);
  if (ret > 0 && FD_ISSET(sock, &fdset)) {
    STOP_CC(select_cycles);
    INCR_OP(select_success);
    return true;
  }
  STOP_CC(select_cycles);
  INCR_OP(select_fail);
  return false;
}

Message *Node::recv() {
  Message *m = new Message(Max_message_size);

  while (1) {
#ifndef ASYNC_SOCK
    while (!has_messages(20000))
      ;
#endif

    INCR_OP(num_recvfrom);
    START_CC(recvfrom_cycles);

    int ret = recvfrom(sock, m->contents(), m->msize(), 0, 0, 0);

    STOP_CC(recvfrom_cycles);

#ifdef LOOSE_MESSAGES
    if (lrand48() % 100 < 4) {
      ret = 0;
    }
#endif

    if (ret >= (int)sizeof(Message_rep) && ret >= m->size()) {
#ifdef ASYNC_SOCK
      ITimer::handle_timeouts();
      INCR_OP(num_recv_success);
      INCR_CNT(bytes_in, m->size());
#endif
      return m;
    }
#ifdef ASYNC_SOCK
    while (!has_messages(20000))
      ;
#endif
  }
}

void Node::gen_auth(char *s, unsigned l, bool in, char *dest) const {
  INCR_OP(num_gen_auth);
  START_CC(gen_auth_cycles);

#ifdef USE_SECRET_SUFFIX_MD5
  // Initialize context with "digest" of message.
  MD5_CTX context, context1;
  MD5Init(&context1);
  MD5Update(&context1, s, l);

  for (int i = 0; i < num_replicas; i++) {
    // Skip myself.
    if (i == node_id) continue;

    memcpy((char *)&context, (char *)&context1, sizeof(MD5_CTX));
    principals[i]->end_mac(&context, dest, in);
    dest += MAC_size;
  }
#else
  long long unonce = Principal::new_umac_nonce();
  memcpy(dest, (char *)&unonce, UNonce_size);
  dest += UNonce_size;

  for (int i = 0; i < num_replicas; i++) {
    // Skip myself.
    if (i == node_id) continue;

    if (in)
      principals[i]->gen_mac_in(s, l, dest, (char *)&unonce);
    else
      principals[i]->gen_mac_out(s, l, dest, (char *)&unonce);
    dest += UMAC_size;
  }
#endif

  STOP_CC(gen_auth_cycles);
}

bool Node::verify_auth(int i, char *s, unsigned l, bool in, char *dest) const {
  th_assert(node_id < num_replicas, "Called by non-replica");

  INCR_OP(num_ver_auth);
  START_CC(ver_auth_cycles);

  Principal *p = i_to_p(i);

  // Principal never verifies its own authenticator.
  if (p != 0 && i != node_id) {
#ifdef USE_SECRET_SUFFIX_MD5
    int offset = node_id * MAC_size;
    if (node_id > i) offset -= MAC_size;
    bool ret = (in) ? p->verify_mac_in(s, l, dest + offset)
                    : p->verify_mac_out(s, l, dest + offset);
#else
    long long unonce;
    memcpy((char *)&unonce, dest, UNonce_size);
    dest += UNonce_size;
    int offset = node_id * UMAC_size;
    if (node_id > i) offset -= UMAC_size;
    bool ret = (in) ? p->verify_mac_in(s, l, dest + offset, (char *)&unonce)
                    : p->verify_mac_out(s, l, dest + offset, (char *)&unonce);
#endif

    STOP_CC(ver_auth_cycles);
    return ret;
  }

  STOP_CC(ver_auth_cycles);
  return false;
}

void Node::gen_signature(const char *src, unsigned src_len, char *sig) {
  INCR_OP(num_sig_gen);
  START_CC(sig_gen_cycles);

  size_t key_size = priv_key->size();
  if (key_size + sizeof(key_size) > sig_size()) {
    th_fail("Signature is too big");
  }

  memcpy(sig, &key_size, sizeof(key_size));
  sig += sizeof(key_size);
  const std::string msg(src, src_len);
  int err = priv_key->sign(msg, reinterpret_cast<uint8_t *>(sig), key_size);
  if (err) {
    th_fail("failed to sign message");
  }

  STOP_CC(sig_gen_cycles);
}

unsigned Node::decrypt(char *src, unsigned src_len, char *dst,
                       unsigned dst_len) {
  if (src_len < 2 * sizeof(unsigned)) return 0;

  unsigned ciphertext_len;
  unsigned plaintext_len;
  memcpy((char *)&plaintext_len, src, sizeof(unsigned));
  src += sizeof(plaintext_len);
  memcpy((char *)&ciphertext_len, src, sizeof(unsigned));
  src += sizeof(ciphertext_len);
  src_len -= (sizeof(plaintext_len) + sizeof(ciphertext_len));

  if (dst_len < plaintext_len || src_len < ciphertext_len) return 0;

  size_t dec_plaintext_len;
  int err = priv_key->decrypt(reinterpret_cast<uint8_t *>(src), ciphertext_len,
                              dst, dst_len, &dec_plaintext_len);
  if (err) {
    th_fail("failed to decrypt message");
  }
  th_assert(plaintext_len == dec_plaintext_len, "unexpected plaintext length");

  return plaintext_len;
}

Request_id Node::new_rid() {
  if ((unsigned)cur_rid == (unsigned)0xffffffff) {
    new_tstamp();
  }
  return ++cur_rid;
}

void Node::new_tstamp() {
  struct timeval t;
  gettimeofday(&t, 0);

  // FIXME: Originally, this checked whether sizeof(tv_sec's) <= sizeof(int).
  // The change here is only a quick fix. We probably should replace the current
  // request ID implementation with a struct or something similar.
  th_assert(t.tv_sec <= INT_MAX, "tv_sec is too big");
  Long tstamp = t.tv_sec;
  long int_bits = sizeof(int) * 8;
  cur_rid = tstamp << int_bits;
}

mbedtls_ctr_drbg_context *Node::drbg_context() { return &this->ctr_drbg_ctx; }

void atimer_handler() {
  th_assert(node, "replica is not initialized\n");

  // Multicast new key to all replicas.
  node->send_new_key();
}

void Node::send_new_key() {
  delete last_new_key;

  // Multicast new key to all replicas.
  last_new_key = new New_key();
  send(last_new_key, All_replicas);

  // Stop timer if not expired and then restart it
  atimer->stop();
  atimer->restart();
}
