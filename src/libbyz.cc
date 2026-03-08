#include "libbyz.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cstring>

#include "Client.h"
#include "Node.h"
#include "Replica.h"
#include "Reply.h"
#include "Request.h"
#include "State_defs.h"
#include "Statistics.h"
#include "mem_statistics_guard.h"
#include "special_region.h"

void Byz_print_memory_consumption(const size_t mem_size) {
  libbyzea::Replica::print_memory_consumption(mem_size);
}

int Byz_init_client(const char* conf, const char* conf_priv, short port) {
#ifdef PRINT_MEM_STATISTICS
  libbyzea::MemoryStatisticsGuard mem_guard("Byz_init_client", true);
#endif
  FILE* config_file = fopen(conf, "r");
  if (config_file == 0) {
    fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
    return -1;
  }

  // FIXME: we should make sure that client initialization was successful in the
  // constructor, by adding a is_valid method or something similar.
  FILE* config_priv_file = fopen(conf_priv, "r");
  if (config_priv_file == 0) {
    fprintf(stderr, "libbyz: Invalid private configuration file %s \n",
            conf_priv);
    return -1;
  }
  fclose(config_priv_file);

  // Initialize random number generator
  srand48(getpid());

  const std::string private_key_file(conf_priv, std::strlen(conf_priv));
  libbyzea::Client* client = new libbyzea::Client(
      MEM_STATS_ARG_PUSH(Client) config_file, private_key_file, port);
  libbyzea::node = client;
  return 0;
}

void Byz_reset_client() { ((libbyzea::Client*)libbyzea::node)->reset(); }

int Byz_alloc_request(Byz_req* req, [[maybe_unused]] int size) {
  libbyzea::Request* request;
#ifdef STATIC_LOG_ALLOCATOR
  if (libbyzea::node->is_replica(libbyzea::node->id())) {
    request =
        libbyzea::special_region::new_request(libbyzea::replica->new_rid());
  } else {
    request = new libbyzea::Request((Request_id)0);
  }
#else
  request = new libbyzea::Request((Request_id)0);
#endif
  if (request == 0) return -1;

  int len;
  req->contents = request->store_command(len);
  req->size = len;
  req->opaque = (void*)request;
  return 0;
}

int Byz_send_request(Byz_req* req, [[maybe_unused]] bool ro) {
  libbyzea::Request* request = (libbyzea::Request*)req->opaque;
  // Replicas already add rid in Byz_alloc_request
  if (!libbyzea::node->is_replica(libbyzea::node->id())) {
    request->request_id() = ((libbyzea::Client*)libbyzea::node)->get_rid();
  } else {
    request->request_id() = libbyzea::replica->new_rid();
  }
  request->sign(req->size);

  bool retval;
  if (!libbyzea::node->is_replica(libbyzea::node->id())) {
    retval = ((libbyzea::Client*)libbyzea::node)->send_request(request);
  } else {
    retval = ((libbyzea::Replica*)libbyzea::node)->send_request(request);
  }
  return (retval) ? 0 : -1;
}

int Byz_recv_reply(Byz_rep* rep) {
  libbyzea::Reply* reply = ((libbyzea::Client*)libbyzea::node)->recv_reply();
  if (reply == NULL) return -1;
  rep->contents = reply->reply(rep->size);
  rep->opaque = reply;
  return 0;
}

int Byz_invoke(Byz_req* req, Byz_rep* rep, bool ro) {
  if (Byz_send_request(req, ro) == -1) return -1;
  return Byz_recv_reply(rep);
}

void Byz_free_request(Byz_req* req) {
  libbyzea::Request* request = (libbyzea::Request*)req->opaque;
#ifdef STATIC_LOG_ALLOCATOR
  if (!libbyzea::node->is_replica(libbyzea::node->id())) {
    delete request;
  }
#else
  delete request;
#endif
}

void Byz_free_reply(Byz_rep* rep) {
  libbyzea::Reply* reply = (libbyzea::Reply*)rep->opaque;
  delete reply;
}

#ifndef NO_STATE_TRANSLATION

int Byz_init_replica(const char* conf, const char* conf_priv,
                     unsigned int num_objs,
                     int (*exec)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool),
                     void (*comp_ndet)(Seqno, Byz_buffer*), int ndet_max_len,
                     bool (*check_ndet)(Byz_buffer*), int (*get)(int, char**),
                     void (*put)(int, int*, int*, char**),
                     void (*shutdown_proc)(FILE* o),
                     void (*restart_proc)(FILE* i), short port) {
  FILE* config_file = fopen(conf, "r");
  if (config_file == 0) {
    fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
    return -1;
  }

  // FIXME: we should make sure that replica initialization was successful in
  // the constructor, by adding a is_valid method or something similar.
  FILE* config_priv_file = fopen(conf_priv, "r");
  if (config_priv_file == 0) {
    fprintf(stderr, "libbyz: Invalid private configuration file %s \n",
            conf_priv);
    return -1;
  }
  fclose(config_priv_file);

  // Initialize random number generator
  srand48(getpid());

  const std::string private_key_file(conf_priv, std::strlen(conf_priv));
  replica = new Replica(config_file, private_key_file, num_objs, get, put,
                        shutdown_proc, restart_proc, port);
  node = replica;

  // Register service-specific functions.
  replica->register_exec(exec);
  replica->register_nondet_choices(comp_ndet, ndet_max_len, check_ndet);
  return replica->used_state_pages();
}

void Byz_modify(int npages, int* pages) {
  for (int i = 0; i < npages; i++) replica->modify_index(pages[i]);
}

#else

int Byz_init_replica(const char* conf, const char* conf_priv, char* mem,
                     unsigned int size,
                     int (*exec)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool),
                     void (*comp_ndet)(Seqno, Byz_buffer*), int ndet_max_len,
                     int (*recv_reply)(Byz_rep*), short port) {
#ifdef PRINT_MEM_STATISTICS
  libbyzea::MemoryStatisticsGuard mem_guard("Byz_init_replica", true);
#endif
  MEM_STATS_GUARD_PUSH(fopen);
  FILE* config_file = fopen(conf, "r");
  if (config_file == 0) {
    fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
    return -1;
  }
  MEM_STATS_GUARD_POP();

  // FIXME: we should make sure that replica initialization was successful in
  // the constructor, by adding a is_valid method or something similar.
  MEM_STATS_GUARD_PUSH(fopen);
  FILE* config_priv_file = fopen(conf_priv, "r");
  if (config_priv_file == 0) {
    fprintf(stderr, "libbyz: Invalid private configuration file %s \n",
            conf_priv);
    return -1;
  }
  fclose(config_priv_file);
  MEM_STATS_GUARD_POP();

  // Initialize random number generator
  srand48(getpid());

#ifdef DYNAMIC_PARTITION_TREE
  fprintf(stderr, "libbyz: Initializing partition tree with size %d\n", size);
  libbyzea::partition::init(size);
#endif

  fprintf(stderr, "libbyz: Creating Replica object...\n");
  const std::string private_key_file(conf_priv, std::strlen(conf_priv));
  libbyzea::replica =
      new libbyzea::Replica(MEM_STATS_ARG_PUSH(Replica) config_file,
                            private_key_file, mem, size, port);
  libbyzea::node = libbyzea::replica;

  // Register service-specific functions.
  libbyzea::replica->register_exec(exec);
  libbyzea::replica->register_nondet_choices(comp_ndet, ndet_max_len);
  libbyzea::replica->register_recv_callback(recv_reply);

  return libbyzea::replica->used_state_bytes();
}

void Byz_modify(char* mem, int size) { libbyzea::replica->modify(mem, size); }

#endif

void Byz_replica_run() {
#ifdef PRINT_MEM_STATISTICS
  libbyzea::MemoryStatisticsGuard mem_guard("Byz_replica_run", true);
  MEMSTATS_RUNTIME_LOGGING(1);
#endif
#ifdef PRINT_STATS
  libbyzea::stats.zero_stats();
#endif
  libbyzea::replica->recv();
}

#ifdef NO_STATE_TRANSLATION
void _Byz_modify_index(int bindex) { libbyzea::replica->modify_index(bindex); }
#endif

void Byz_reset_stats() {
#ifdef PRINT_STATS
  libbyzea::stats.zero_stats();
#else
  fprintf(stderr,
          "WARNING: libbyzea was not compiled with statistics support. Please "
          "recompile with -DPRINT_STATS=1\n");
#endif
}

void Byz_print_stats() {
#ifdef PRINT_STATS
  libbyzea::stats.print_stats();
#else
  fprintf(stderr,
          "WARNING: libbyzea was not compiled with statistics support. Please "
          "recompile with -DPRINT_STATS=1\n");
#endif
}

#ifndef NO_STATE_TRANSLATION
char* Byz_get_cached_object(int i) { return replica->get_cached_obj(i); }
#endif
