#include "Replica.h"

#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Checkpoint.h"
#include "Commit.h"
#include "Data.h"
#include "Fetch.h"
#include "ITimer.h"
#include "K_max.h"
#include "Message_tags.h"
#include "Meta_data.h"
#include "Meta_data_d.h"
#include "New_key.h"
#include "New_view.h"
#include "Pre_prepare.h"
#include "Prepare.h"
#include "Prepared_cert.h"
#include "Principal.h"
#include "Query_stable.h"
#include "Reply.h"
#include "Reply_stable.h"
#include "Request.h"
#include "State_defs.h"
#include "Statistics.h"
#include "Status.h"
#include "View_change.h"
#include "View_change_ack.h"
#include "checkpoint_record.h"
#include "checkpoint_record_log.h"
#include "th_assert.h"

// TODO check if includes are necessary at their positions or can be moved to
// regular includes.

namespace libbyzea {
// Global replica object.
Replica *replica;
}  // namespace libbyzea

// Force template instantiation
#include "Array.t"
namespace libbyzea {
template class Array<libbyzea::Req_queue::PNode>;
}

#include "Certificate.t"
namespace libbyzea {
template class Certificate<Commit>;
template class Certificate<Checkpoint>;
template class Certificate<Reply>;
}  // namespace libbyzea

#include "Log.t"
namespace libbyzea {
template class Log<Prepared_cert>;
template class Log<Certificate<Commit>>;
template class Log<Certificate<Checkpoint>>;
}  // namespace libbyzea

#include "Set.t"
namespace libbyzea {
template class Set<Checkpoint>;
}  // namespace libbyzea

namespace libbyzea {

template <class T>
void Replica::retransmit(T *m, Time &cur, Time *tsent, Principal *p) {
  // XXXXXXXXXXXthere most be a bug in the way tsent is managed. Figure out
  //  where and reinsert this protection against denial of service attacks.
  // if (diffTime(cur, *tsent) > 1000) {
  if (1) {
    // if (p->is_stale(tsent)) {
    if (1) {
      // Authentication for this principal is stale in message -
      // re-authenticate.
      m->re_authenticate(p);
    }

    //    printf("RET: %s to %d \n", m->stag(), p->pid());

    // Retransmit message
    send(m, p->pid());

    *tsent = cur;
  }
}

void Replica::print_memory_consumption([[maybe_unused]] const size_t mem_size) {
#if 0
  State::print_memory_consumption(mem_size);

  size_t size = sizeof(Replica) + 4 * sizeof(ITimer) + 4 * sizeof(View) +
                4 * sizeof(Checkpoint *);
  fprintf(stderr, "Replica: %lu\n", size);
  size = sizeof(Node) + sizeof(RsaPrivateKey) + sizeof(ITimer) +
         // sizeof(umac_ctx) = 1528
         6 * (sizeof(Principal) +
              (sizeof(RsaPublicKey) + sizeof(mbedtls_rsa_context)) + 1528);
  fprintf(stderr, "Node: %lu\n", size);

  size = Log<Prepared_cert>::memory_consumption(max_out) +
         sizeof(Log<Prepared_cert>);
  fprintf(stderr, "Log<Prepared_cert>: %lu\n", size);
  size = Log<Certificate<Commit>>::memory_consumption(max_out) +
         sizeof(Log<Certificate<Commit>>);
  fprintf(stderr, "Log<Certificate<Commit>>: %lu\n", size);
  size = Log<Certificate<Checkpoint>>::memory_consumption(2 * max_out) +
         sizeof(Log<Certificate<Checkpoint>>);
  fprintf(stderr, "Log<Certificate<Checkpoint>>: %lu\n", size);

  fprintf(stderr, "Pre-Prepare: %lu\n", Max_message_size);
  fprintf(stderr, "Prepare: %lu\n",
          sizeof(Prepare) + sizeof(Prepare_rep) + (4 * (MAC_size)));
  fprintf(stderr, "Commit: %lu\n",
          sizeof(Commit) + sizeof(Commit_rep) + (4 * (MAC_size)));
  fprintf(stderr, "Checkpoint: %lu\n",
          sizeof(Checkpoint) + sizeof(Checkpoint_rep) + (4 * (MAC_size)));
#endif

  fprintf(stderr, "\n\n\n");
  fprintf(stderr, "sizeof(Replica) = %zu\n", sizeof(Replica));
  fprintf(stderr, "sizeof(Node) = %zu\n", sizeof(Node));
  fprintf(stderr, "sizeof(Prepared_cert) = %zu\n", sizeof(Prepared_cert));
  fprintf(stderr, "sizeof(State) = %zu\n", sizeof(State));
  fprintf(stderr, "sizeof(View_info) = %zu\n", sizeof(View_info));
  fprintf(stderr, "sizeof(CheckpointRecordLog) = %zu\n",
          sizeof(CheckpointRecordLog));
  // fprintf(stderr, "sizeof(Checkpoint_rec) = %u\n", sizeof(Checkpoint_rec));
  fprintf(stderr, "sizeof(CheckpointRecord) = %zu\n", sizeof(CheckpointRecord));
  fprintf(stderr, "sizeof(Log<Prepared_cert>) = %zu\n",
          sizeof(Log<Prepared_cert>));
  fprintf(stderr, "sizeof(Certificate<Commit>) = %zu\n",
          sizeof(Certificate<Commit>));
  fprintf(stderr, "sizeof(Log<Certificate<Commit>>) = %zu\n",
          sizeof(Log<Certificate<Commit>>));
  fprintf(stderr, "sizeof(Certificate<Checkpoint>) = %zu\n",
          sizeof(Certificate<Checkpoint>));
  fprintf(stderr, "sizeof(Log<Certificate<Checkpoint>>) = %zu\n",
          sizeof(Log<Certificate<Checkpoint>>));
  fprintf(stderr, "sizeof(CheckpointLog) = %zu\n", sizeof(CheckpointLog));
  fprintf(stderr, "sizeof(OReq_info) = %zu", sizeof(View_info::OReq_info));
#ifdef STATIC_LOG_ALLOCATOR
  fprintf(stderr, "sizeof(agreement_region) = %zu\n",
          agreement_region::memory_demand());
  fprintf(stderr, "sizeof(checkpoint_region) = %zu\n",
          checkpoint_region::memory_demand());
  fprintf(stderr, "sizeof(special_region) = %zu\n",
          special_region::memory_demand());
  fprintf(stderr, "sizeof(scratch_region) = %zu\n",
          scratch_allocator::memory_demand());
#endif
}

#ifndef NO_STATE_TRANSLATION

Replica::Replica(FILE *config_file, const std::string &private_key_file,
                 int num_objs, int (*get)(int, char **),
                 void (*put)(int, int *, int *, char **),
                 void (*shutdown_proc)(FILE *o), void (*restart_proc)(FILE *i),
                 short port)
    : Node(config_file, config_priv, port),
      rqueue(),
      ro_rqueue(),
      plog(max_out),
      clog(max_out),
      elog(max_out * 2, 0),
      sset(n()),
      replies(num_principals),
      state(this, num_objs, get, put, shutdown_proc, restart_proc),
      vi(node_id, 0),
      n_mem_blocks(num_objs) {
#else

Replica::Replica(MEM_STATS_PARAM FILE *config_file,
                 const std::string &private_key_file, char *mem, int nbytes,
                 short port)
    : Node(MEM_STATS_ARG_PUSH(Node) config_file, private_key_file, port),
      rqueue(MEM_STATS_GUARD_PUSH(Req_queue)),
      ro_rqueue(MEM_STATS_GUARD_PUSH(Req_queue)),
      plog(MEM_STATS_ARG_PUSH(Log<Prepared_cert>) max_out),
      clog(MEM_STATS_ARG_PUSH(Log<Certificate<Commit>>) max_out),
#ifdef ALTERNATIVE_CHECKPOINT_LOG
      elog(MEM_STATS_GUARD_PUSH(CheckpointLog)),
#else
      elog(MEM_STATS_ARG_PUSH(Log < Certificate<Checkpoint>) max_out * 2, 0),
#endif
      sset(n()),
      replies(mem, nbytes, num_principals),
      state(MEM_STATS_ARG_PUSH(State) this, mem, nbytes),
      vi(MEM_STATS_ARG_PUSH(View_info) node_id, 0),
      rr_reps(MEM_STATS_GUARD_PUSH(Certificate<Reply>)) {

#endif
  // Fail if node is not a replica.
  if (!is_replica(id())) th_fail("Node is not a replica");

  seqno = 0;
  last_stable = 0;
  low_bound = 0;

  last_prepared = 0;
  last_executed = 0;
  last_tentative_execute = 0;

  last_status = zeroTime();

  limbo = false;
  has_nv_state = true;

  nbreqs = 0;
  nbrounds = 0;

  // Read view change, status, and recovery timeouts from replica's portion
  // of "config_file"
  int vt, st, rt;
  fscanf(config_file, "%d\n", &vt);
  fscanf(config_file, "%d\n", &st);
  fscanf(config_file, "%d\n", &rt);

  // Create timers and randomize times to avoid collisions.
  srand48(getpid());

  vtimer = new ITimer(vt + lrand48() % 100, vtimer_handler);
  stimer = new ITimer(st + lrand48() % 100, stimer_handler);

  // Skew recoveries. It is important for nodes to recover in the reverse order
  // of their node ids to avoid a view-change every recovery which would degrade
  // performance.
  rtimer = new ITimer(rt, rec_timer_handler);
  rec_ready = false;
  rtimer->start();

  ntimer = new ITimer(30000 / max_out, ntimer_handler);

  recovering = false;
  qs = 0;
  rr = 0;
  rr_views = new View[num_replicas];
  recovery_point = Seqno_max;
  max_rec_n = 0;

  exec_command = 0;
  non_det_choices = 0;

  join_mcast_group();

  // Disable loopback
  u_char l = 0;
  int error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &l, sizeof(l));
  if (error < 0) {
    perror("unable to disable loopback");
    exit(1);
  }

#ifdef LARGE_SND_BUFF
  int snd_buf_size = 262144;
  error = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&snd_buf_size,
                     sizeof(snd_buf_size));
  if (error < 0) {
    perror("unable to increase send buffer size");
    exit(1);
  }
#endif

#ifdef LARGE_RCV_BUFF
  int rcv_buf_size = 131072;
  error = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_buf_size,
                     sizeof(rcv_buf_size));
  if (error < 0) {
    perror("unable to increase recv buffer size");
    exit(1);
  }
#endif
  MEM_STATS_GUARD_POP();
}

void Replica::register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int,
                                     bool)) {
  exec_command = e;
}

#ifndef NO_STATE_TRANSLATION
void Replica::register_nondet_choices(void (*n)(Seqno, Byz_buffer *),
                                      int max_len,
                                      bool (*check)(Byz_buffer *)) {
  check_non_det = check;
#else
void Replica::register_nondet_choices(void (*n)(Seqno, Byz_buffer *),
                                      int max_len) {
#endif
  non_det_choices = n;
  max_nondet_choice_len = max_len;
}

void Replica::compute_non_det(Seqno s, char *b, int *b_len) {
  if (non_det_choices == 0) {
    *b_len = 0;
    return;
  }
  Byz_buffer buf;
  buf.contents = b;
  buf.size = *b_len;
  non_det_choices(s, &buf);
  *b_len = buf.size;
}

Replica::~Replica() {}

void Replica::recv() {
  MEMSTATS_MEM_TYPE_VAR
  // Compute session keys and send initial new-key message.
  Node::send_new_key();

  // Compute digest of initial state and first checkpoint.
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.compute_full_digest();
  MEMSTATS_RESTORE_MEM_TYPE();

  // Start status and authentication freshness timers
  stimer->start();
  atimer->start();
  if (id() == primary()) ntimer->start();

  // Allow recoveries
  rec_ready = true;

  fprintf(stderr, "Replica ready\n");

  while (1) {
    if (state.in_check_state()) {
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
      state.check_state();
      MEMSTATS_RESTORE_MEM_TYPE();
    }

    Message *m = Node::recv();
    if (qs) {
      if (m->tag() != New_key_tag && m->tag() != Query_stable_tag &&
          m->tag() != Reply_stable_tag && m->tag() != Status_tag) {
        // While estimating replica only handles certain messages.
        delete m;
        continue;
      }
    }

#if 1
    if (state.in_check_state()) {
      if (m->tag() < 6 && m->tag() != Reply_tag) {
        delete m;
        continue;
      }
    }
#endif

    // TODO: This should probably be a jump table.
    // fprintf(stderr, "%s\n", m->stag());
    switch (m->tag()) {
      case Request_tag:
        gen_handle<Request>(m);
        break;

      case Pre_prepare_tag:
        gen_handle<Pre_prepare>(m);
        break;

      case Prepare_tag:
        gen_handle<Prepare>(m);
        break;

      case Commit_tag:
        gen_handle<Commit>(m);
        break;

      case Checkpoint_tag:
        gen_handle<Checkpoint>(m);
        break;

      case New_key_tag:
        gen_handle<New_key>(m);
        break;

      case View_change_ack_tag:
        gen_handle<View_change_ack>(m);
        break;

      case Status_tag:
        gen_handle<Status>(m);
        break;

      case Fetch_tag:
        gen_handle<Fetch>(m);
        break;

      case Reply_tag:
        gen_handle<Reply>(m);
        break;

      case Query_stable_tag:
        gen_handle<Query_stable>(m);
        break;

      case Reply_stable_tag:
        gen_handle<Reply_stable>(m);
        break;

      case Meta_data_tag:
        gen_handle<Meta_data>(m);
        break;

      case Meta_data_d_tag:
        gen_handle<Meta_data_d>(m);
        break;

      case Data_tag:
        gen_handle<Data>(m);
        break;

      case View_change_tag:
        gen_handle<View_change>(m);
        break;

      case New_view_tag:
        gen_handle<New_view>(m);
        break;

      default:
        fprintf(stderr, "Unknown.\n");
        // Unknown message type.
        delete m;
    }
  }
}

void Replica::handle(Request *m) {
  int cid = m->client_id();
  bool ro = m->is_read_only();
  Request_id rid = m->request_id();

  if (has_new_view() && m->verify()) {
    // Replica's requests must be signed and cannot be read-only.
    if (!is_replica(cid) || !ro) {
      if (ro) {
        // Read-only requests.
        if (execute_read_only(m) || !ro_rqueue.append(m)) delete m;
        return;
      }

      Request_id last_rid = replies.req_id(cid);
      if (last_rid < rid) {
        // Request has not been executed.
        if (id() == primary()) {
          if (!rqueue.in_progress(cid, rid, v) && rqueue.append(m)) {
            //	    fprintf(stderr, "RID %qd. ", rid);
#ifdef STATIC_LOG_ALLOCATOR
            delete m;
#endif
            send_pre_prepare();
            return;
          }
        } else {
          if (rqueue.append(m)) {
            if (!limbo) {
              send(m, primary());
              vtimer->start();
            }
#ifdef STATIC_LOG_ALLOCATOR
            delete m;
#endif
            return;
          }
        }
      } else if (last_rid == rid) {
        // Retransmit reply.
        replies.send_reply(cid, view(), id());

        if (id() != primary() && !replies.is_committed(cid) &&
            rqueue.append(m)) {
#ifdef STATIC_LOG_ALLOCATOR
          delete m;
#endif
          vtimer->start();
          return;
        }
      }
    }
  }

  delete m;
}

void Replica::send_pre_prepare() {
  MEMSTATS_MEM_TYPE_VAR
  th_assert(primary() == node_id, "Non-primary called send_pre_prepare");

  // If rqueue is empty there are no requests for which to send
  // pre_prepare and a pre-prepare cannot be sent if the seqno excedes
  // the maximum window or the replica does not have the new view.
  if (rqueue.size() > 0 && seqno + 1 <= last_executed + congestion_window &&
      seqno + 1 <= max_out + last_stable && has_new_view()) {
    //  printf("requeu.size = %d\n", rqueue.size());
    //  if (seqno % checkpoint_interval == 0)
    // printf("SND: PRE-PREPARE seqno=%qd last_stable=%qd\n", seqno+1,
    // last_stable);

    nbreqs += rqueue.size();
    nbrounds++;

    /*
    if (nbreqs >= 10000) {
            printf("Avg batch sz: %f\n", (float)nbreqs/(float)nbrounds);
            nbreqs = nbrounds = 0;
    }
    */

    // Create new pre_prepare message for set of requests
    // in rqueue, log message and multicast the pre_prepare.
    seqno++;
    //    fprintf(stderr, "Sending PP seqno %qd\n", seqno);
    Pre_prepare *pp = new Pre_prepare(view(), seqno, rqueue);

    // TODO: should make code match my proof with request removed
    // only when executed rather than removing them from rqueue when the
    // pre-prepare is constructed.

    send(pp, All_replicas);
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    plog.fetch(seqno).add_mine(pp);
    MEMSTATS_RESTORE_MEM_TYPE();
#ifdef STATIC_LOG_ALLOCATOR
    delete pp;
#endif
  }
}

template <class T>
bool Replica::in_w(T *m) {
  const Seqno offset = m->seqno() - last_stable;

  if (offset > 0 && offset <= max_out) return true;

  if (offset > max_out && m->verify()) {
    // Send status message to obtain missing messages. This works as a
    // negative ack.
    send_status();
  }

  return false;
}

template <class T>
bool Replica::in_wv(T *m) {
  const Seqno offset = m->seqno() - last_stable;

  if (offset > 0 && offset <= max_out && m->view() == view()) return true;

  if ((m->view() > view() || offset > max_out) && m->verify()) {
    // Send status message to obtain missing messages. This works as a
    // negative ack.
    send_status();
  }

  return false;
}

void Replica::handle(Pre_prepare *m) {
  MEMSTATS_MEM_TYPE_VAR
  const Seqno ms = m->seqno();

  Byz_buffer b;

  b.contents = m->choices(b.size);

#ifndef NO_STATE_TRANSLATION
  if (in_wv(m) && ms > low_bound && has_new_view() && check_non_det(&b)) {
#else
  if (in_wv(m) && ms > low_bound && has_new_view()) {
#endif
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    Prepared_cert &pc = plog.fetch(ms);
    MEMSTATS_RESTORE_MEM_TYPE();

    // Only accept message if we never accepted another pre-prepare
    // for the same view and sequence number and the message is valid.
    if (pc.add(m)) {
#ifdef STATIC_LOG_ALLOCATOR
      delete m;
#endif
      send_prepare(pc);
      if (pc.is_complete()) send_commit(ms);
    }
    return;
  }

  if (!has_new_view()) {
    // This may be an old pre-prepare that replica needs to complete
    // a view-change.
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
    vi.add_missing(m);
    MEMSTATS_RESTORE_MEM_TYPE();
    return;
  }

  delete m;
}

void Replica::send_prepare(Prepared_cert &pc) {
  if (pc.my_prepare() == 0 && pc.is_pp_complete()) {
    // Send prepare to all replicas and log it.
    Pre_prepare *pp = pc.pre_prepare();
    Prepare *p = new Prepare(v, pp->seqno(), pp->digest());
    send(p, All_replicas);
    if (pc.add_mine(p)) {
#ifdef STATIC_LOG_ALLOCATOR
      delete p;
#endif
    }
  }
}

void Replica::send_commit(Seqno s) {
  MEMSTATS_MEM_TYPE_VAR
  // Executing request before sending commit improves performance
  // for null requests. May not be true in general.
  if (s == last_executed + 1) execute_prepared();

  Commit *c = new Commit(view(), s);
  send(c, All_replicas);

  if (s > last_prepared) last_prepared = s;

  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  Certificate<Commit> &cs = clog.fetch(s);
  auto added = cs.add_mine(c);
#ifdef STATIC_LOG_ALLOCATOR
  if (added) {
    delete c;
  }
#endif
  if (added && cs.is_complete()) {
    MEMSTATS_RESTORE_MEM_TYPE();
    execute_committed();
  }
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::handle(Prepare *m) {
  MEMSTATS_MEM_TYPE_VAR
  const Seqno ms = m->seqno();

  // Only accept prepare messages that are not sent by the primary for
  // current view.
  if (in_wv(m) && ms > low_bound && primary() != m->id() && has_new_view()) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    Prepared_cert &ps = plog.fetch(ms);
    if (ps.add(m)) {
      if (ps.is_complete()) {
        send_commit(ms);
      }
#ifdef STATIC_LOG_ALLOCATOR
      delete m;
#endif
    }
    MEMSTATS_RESTORE_MEM_TYPE();
    return;
  }

  if (m->is_proof() && !has_new_view()) {
    // This may be an prepare sent to prove the authenticity of a
    // request to complete a view-change.
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
    vi.add_missing(m);
    MEMSTATS_RESTORE_MEM_TYPE();
    return;
  }

  delete m;
  return;
}

void Replica::handle(Commit *m) {
  MEMSTATS_MEM_TYPE_VAR
  const Seqno ms = m->seqno();

  // Only accept messages with the current view.  TODO: change to
  // accept commits from older views as in proof.
  if (in_wv(m) && ms > low_bound) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    Certificate<Commit> &cs = clog.fetch(m->seqno());
    auto added = cs.add(m);
#if STATIC_LOG_ALLOCATOR
    if (added) {
      delete m;
    }
#endif
    if (added && cs.is_complete()) {
      MEMSTATS_RESTORE_MEM_TYPE();
      execute_committed();
    }
    MEMSTATS_RESTORE_MEM_TYPE();
    return;
  }
  delete m;
  return;
}

void Replica::handle(Checkpoint *m) {
  MEMSTATS_MEM_TYPE_VAR
  const Seqno ms = m->seqno();
  if (ms > last_stable) {
    if (ms <= last_stable + max_out) {
      // Checkpoint is within my window.  Check if checkpoint is
      // stable and it is above my last_executed.  This may signal
      // that messages I missed were garbage collected and I should
      // fetch the state.
      bool late = m->stable() && last_executed < ms;
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
      if (clog.within_range(last_executed)) {
        Time *t = nullptr;
        clog.fetch(last_executed).mine(&t);
        MEMSTATS_RESTORE_MEM_TYPE();
        late &= diffTime(currentTime(), *t) > 200000;
      }
      MEMSTATS_RESTORE_MEM_TYPE();

      if (!late) {
        Certificate<Checkpoint> &cs = elog.fetch(ms);
        auto added = cs.add(m);
        if (added) {
#ifdef STATIC_LOG_ALLOCATOR
          delete m;
#endif

          if (cs.mine() && cs.is_complete()) {
            // I have enough Checkpoint messages for m->seqno() to make it
            // stable. Truncate logs, discard older stable state versions.
            //	  fprintf(stderr, "CP MSG call MS %qd!!!\n", last_executed);
            mark_stable(ms, true);
          }
        }
        return;
      }
    }

    if (m->verify()) {
      // Checkpoint message above my window.

      if (!m->stable()) {
        // Send status message to obtain missing messages. This works as a
        // negative ack.
        send_status();
        delete m;
        return;
      }

      // Stable checkpoint message above my last_executed.
      Checkpoint *c = sset.fetch(m->id());
      if (c == nullptr || c->seqno() < ms) {
        delete sset.remove(m->id());
#ifdef STATIC_LOG_ALLOCATOR
        auto tmp = m->persist(m->id());
        delete m;
        m = tmp;
#endif
        sset.store(m);
        if (sset.size() > f()) {
          if (last_tentative_execute > last_executed) {
            // Rollback to last checkpoint
            th_assert(!state.in_fetch_state(), "Invalid state");
            MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
            Seqno rc = state.rollback();
            MEMSTATS_RESTORE_MEM_TYPE();
            last_tentative_execute = last_executed = rc;
            //	    fprintf(stderr, ":):):):):):):):) Set le = %d\n",
            // last_executed);
          }

          // Stop view change timer while fetching state. It is restarted
          // in new state when the fetch ends.
          vtimer->stop();
          MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
          state.start_fetch(last_executed);
          MEMSTATS_RESTORE_MEM_TYPE();
        }
        return;
      }
    }
  }
  delete m;
  return;
}

void Replica::handle(New_key *m) {
  if (m->id() != node_id && !m->verify()) {
    fprintf(stderr, "BAD NKEY from %d\n", m->id());
  }
  delete m;
}

void Replica::handle(Status *m) {
  MEMSTATS_MEM_TYPE_VAR
  static const int max_ret_bytes = 65536;

  // Ignore our own status messages, otherwise we would spam New_key messages to
  // ourselves.
  if (m->id() == id()) {
    delete m;
    return;
  }

  if (m->verify() && qs == 0) {
    Time current;
    Time *t;
    current = currentTime();
    Principal *p = node->i_to_p(m->id());

    // Retransmit messages that the sender is missing.
    if (last_stable > m->last_stable() + max_out) {
      // Node is so out-of-date that it will not accept any
      // pre-prepare/prepare/commmit messages in my log.
      // Send a stable checkpoint message for my stable checkpoint.
      Checkpoint *c = elog.fetch(last_stable).mine(&t);
      th_assert(c != 0 && c->stable(), "Invalid state");
      retransmit(c, current, t, p);
      delete m;
      return;
    }

    // Retransmit any checkpoints that the sender may be missing.
    int max = MIN(last_stable, m->last_stable()) + max_out;
    int min = MAX(last_stable, m->last_stable() + 1);
    for (Seqno n = min; n <= max; n++) {
      if (n % checkpoint_interval == 0) {
        Checkpoint *c = elog.fetch(n).mine(&t);
        if (c != 0) {
          retransmit(c, current, t, p);
          th_assert(n == last_stable || !c->stable(), "Invalid state");
        }
      }
    }

    if (m->view() < v) {
      // Retransmit my latest view-change message
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
      View_change *vc = vi.my_view_change(&t);
      MEMSTATS_RESTORE_MEM_TYPE();
      if (vc != 0) retransmit(vc, current, t, p);
      delete m;
      return;
    }

    if (m->view() == v) {
      if (m->has_nv_info()) {
        min = MAX(last_stable + 1, m->last_executed() + 1);
        for (Seqno n = min; n <= max; n++) {
          if (m->is_committed(n)) {
            // No need for retransmission of commit or pre-prepare/prepare
            // message.
            continue;
          }

          MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
          Commit *c = clog.fetch(n).mine(&t);
          MEMSTATS_RESTORE_MEM_TYPE();
          if (c != 0) {
            retransmit(c, current, t, p);
          }

          if (m->is_prepared(n)) {
            // No need for retransmission of pre-prepare/prepare message.
            continue;
          }

          // If I have a pre-prepare/prepare send it, provided I have sent
          // a pre-prepare/prepare for view v.
          if (primary() == node_id) {
            MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
            Pre_prepare *pp = plog.fetch(n).my_pre_prepare(&t);
            MEMSTATS_RESTORE_MEM_TYPE();
            if (pp != 0) {
              retransmit(pp, current, t, p);
            }
          } else {
            MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
            Prepare *pr = plog.fetch(n).my_prepare(&t);
            MEMSTATS_RESTORE_MEM_TYPE();
            if (pr != 0) {
              retransmit(pr, current, t, p);
            }
          }
        }
      } else {
        // m->has_nv_info() == false
        if (!m->has_vc(node_id)) {
          // p does not have my view-change: send it.
          MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
          View_change *vc = vi.my_view_change(&t);
          MEMSTATS_RESTORE_MEM_TYPE();
          th_assert(vc != 0, "Invalid state");
          retransmit(vc, current, t, p);
        }

        if (!m->has_nv_m()) {
          if (primary(v) == node_id && vi.has_new_view(v)) {
            // p does not have new-view message and I am primary: send it
            MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
            New_view *nv = vi.my_new_view(&t);
            MEMSTATS_RESTORE_MEM_TYPE();
            if (nv != 0) retransmit(nv, current, t, p);
          }
        } else {
          if (primary(v) == node_id && vi.has_new_view(v)) {
            // Send any view-change messages that p may be missing
            // that are referred to by the new-view message.  This may
            // be important if the sender of the original message is
            // faulty.
            // XXXXXXXXXX

          } else {
            // Send any view-change acks p may be missing.
            for (int i = 0; i < num_replicas; i++) {
              if (m->id() == i) continue;
              MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
              View_change_ack *vca = vi.my_vc_ack(i);
              MEMSTATS_RESTORE_MEM_TYPE();
              if (vca && !m->has_vc(i))
                // View-change acks are not being authenticated
                retransmit(vca, current, &current, p);
            }
          }

          // Send any pre-prepares that p may be missing and any proofs
          // of authenticity for associated requests.
          Status::PPS_iter gen(m);

          int count = 0;
          Seqno ppn;
          View ppv;
          bool ppp;
          BR_map mrmap;
          while (gen.get(ppv, ppn, mrmap, ppp)) {
            Pre_prepare *pp = 0;
            if (m->id() == primary(v)) {
              MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
              pp = vi.pre_prepare(ppn, ppv);
              MEMSTATS_RESTORE_MEM_TYPE();
            } else {
              MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
              if (primary(v) == id() && plog.within_range(ppn))
                pp = plog.fetch(ppn).pre_prepare();
              MEMSTATS_RESTORE_MEM_TYPE();
            }

            if (pp) {
              retransmit(pp, current, &current, p);

              if (count < max_ret_bytes && mrmap != ~0) {
                Pre_prepare_info pi;
                pi.add_complete(pp);
                pi.zero();  // Make sure pp does not get deallocated
              }
            }

            if (ppp) {
              MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
              vi.send_proofs(ppn, ppv, m->id());
              MEMSTATS_RESTORE_MEM_TYPE();
            }
          }
        }
      }
    }
  } else {
    // It is possible that we could not verify message because the
    // sender did not receive my last new_key message. It is also
    // possible message is bogus. We choose to retransmit last new_key
    // message.  TODO: should impose a limit on the frequency at which
    // we are willing to do it to prevent a denial of service attack.
    // This is not being done right now.
    if (last_new_key != 0 && (qs == 0 || !m->verify())) {
      send(last_new_key, m->id());
    }
  }

  delete m;
}

void Replica::handle(View_change *m) {
  MEMSTATS_MEM_TYPE_VAR
  printf("RECV: view change v=%qd from %d\n", m->view(), m->id());
  if (m->id() == primary() && m->view() > v) {
    printf("Received View_change from primary %d\n", m->id());
    if (m->verify()) {
      printf("Verified\n");
      // "m" was sent by the primary for v and has a view number
      // higher than v: move to the next view.
      send_view_change();
    }
  }

  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  bool modified = vi.add(m);
  MEMSTATS_RESTORE_MEM_TYPE();
  if (!modified) {
    return;
  } else {
#ifdef STATIC_LOG_ALLOCATOR
    delete m;
#endif
  }

  // TODO: memoize maxv and avoid this computation if it cannot change i.e.
  // m->view() <= last maxv. This also holds for the next check.
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  View maxv = vi.max_view();
  MEMSTATS_RESTORE_MEM_TYPE();
  if (maxv > v) {
    // Replica has at least f+1 view-changes with a view number
    // greater than or equal to maxv: change to view maxv.
    v = maxv - 1;
    vc_recovering = true;
    send_view_change();

    return;
  }

  if (limbo && primary() != node_id) {
    maxv = vi.max_maj_view();
    th_assert(maxv <= v, "Invalid state");

    if (maxv == v) {
      // Replica now has at least 2f+1 view-change messages with view  greater
      // than or equal to "v"

      // Start timer to ensure we move to another view if we do not
      // receive the new-view message for "v".
      vtimer->restart();
      limbo = false;
      vc_recovering = true;
    }
  }
}

void Replica::handle(New_view *m) {
  //  printf("RECV: new view v=%qd from %d\n", m->view(), m->id());

  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  vi.add(m);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::handle(View_change_ack *m) {
  //  printf("RECV: view-change ack v=%qd from %d for %d\n", m->view(), m->id(),
  //  m->vc_id());

  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  vi.add(m);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::send_view_change() {
  // Move to next view.
  MEMSTATS_MEM_TYPE_VAR
  v++;
  cur_primary = v % num_replicas;
  limbo = true;
  vtimer->stop();  // stop timer if it is still running
  ntimer->restop();

  if (last_tentative_execute > last_executed) {
    // Rollback to last checkpoint
    th_assert(!state.in_fetch_state(), "Invalid state");
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
    Seqno rc = state.rollback();
    MEMSTATS_RESTORE_MEM_TYPE();
    //    printf("XXXRolled back in vc to %qd with last_executed=%qd\n", rc,
    //    last_executed);
    last_tentative_execute = last_executed = rc;
    //    fprintf(stderr, ":):):):):):):):) Set le = %d\n", last_executed);
  }

  last_prepared = last_executed;

  for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    Prepared_cert &pc = plog.fetch(i);
    Certificate<Commit> &cc = clog.fetch(i);
    MEMSTATS_RESTORE_MEM_TYPE();

    if (pc.is_complete()) {
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
      vi.add_complete(pc.rem_pre_prepare());
      MEMSTATS_RESTORE_MEM_TYPE();
    } else {
      Prepare *p = pc.my_prepare();
      if (p != 0) {
        MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
        vi.add_incomplete(i, p->digest());
        MEMSTATS_RESTORE_MEM_TYPE();
      } else {
        Pre_prepare *pp = pc.my_pre_prepare();
        if (pp != 0) {
          MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
          vi.add_incomplete(i, pp->digest());
          MEMSTATS_RESTORE_MEM_TYPE();
        }
      }
    }

    pc.clear();
    cc.clear();
    // TODO: Could remember info about committed requests for efficiency.
  }

  // Create and send view-change message.
  printf("XXX SND: view change %qd\n", v);
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  vi.view_change(v, last_executed, &state);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::process_new_view(Seqno min, Digest d, Seqno max, Seqno ms) {
  th_assert(ms >= 0 && ms <= min, "Invalid state");
  //  printf("XXX process new view: %qd\n", v);
  MEMSTATS_MEM_TYPE_VAR

  vtimer->restop();
  limbo = false;
  vc_recovering = true;

  if (primary(v) == id()) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
    New_view *nv = vi.my_new_view();
    MEMSTATS_RESTORE_MEM_TYPE();
    send(nv, All_replicas);
  }

  // Setup variables used by mark_stable before calling it.
  seqno = max - 1;
  if (last_stable > min) min = last_stable;
  low_bound = min;

  if (ms > last_stable) {
    // Call mark_stable to ensure there is space for the pre-prepares
    // and prepares that are inserted in the log below.
    mark_stable(ms, last_executed >= ms);
  }

  // Update pre-prepare/prepare logs.
  th_assert(min >= last_stable && max - last_stable - 1 <= max_out,
            "Invalid state");
  for (Seqno i = min + 1; i < max; i++) {
    Digest d;
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
    Pre_prepare *pp = vi.fetch_request(i, d);
    MEMSTATS_RESTORE_MEM_TYPE();
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    Prepared_cert &pc = plog.fetch(i);

    if (primary() == id()) {
      pc.add_mine(pp);
    } else {
      Prepare *p = new Prepare(v, i, d);
      pc.add_mine(p);
      send(p, All_replicas);

      th_assert(pp != nullptr && pp->digest() == p->digest(), "Invalid state");
      pc.add_old(pp);
#ifdef STATIC_LOG_ALLOCATOR
      delete p;
#endif
    }
    MEMSTATS_RESTORE_MEM_TYPE();
  }

  if (primary() == id()) {
    send_pre_prepare();
    ntimer->start();
  }

  if (last_executed < min) {
    has_nv_state = false;
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
    state.start_fetch(last_executed, min, &d, min <= ms);
    MEMSTATS_RESTORE_MEM_TYPE();
  } else {
    has_nv_state = true;

    // Execute any buffered read-only requests
    for (Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove()) {
      execute_read_only(m);
      delete m;
    }
  }

  if (primary() != id() && rqueue.size() > 0) vtimer->restart();

  //  printf("XXX DONE:process new view: %qd\n", v);
}

Pre_prepare *Replica::prepared(Seqno n) {
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  Prepared_cert &pc = plog.fetch(n);
  if (pc.is_complete()) {
    auto *pp = pc.pre_prepare();
    MEMSTATS_RESTORE_MEM_TYPE();
    return pp;
  }
  MEMSTATS_RESTORE_MEM_TYPE();
  return 0;
}

Pre_prepare *Replica::committed(Seqno s) {
  MEMSTATS_MEM_TYPE_VAR
  // TODO: This is correct but too conservative: fix to handle case
  // where commit and prepare are not in same view; and to allow
  // commits without prepared requests, i.e., only with the
  // pre-prepare.
  Pre_prepare *pp = prepared(s);
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  if (clog.fetch(s).is_complete()) {
    MEMSTATS_RESTORE_MEM_TYPE();
    return pp;
  }
  MEMSTATS_RESTORE_MEM_TYPE();
  return 0;
}

bool Replica::execute_read_only(Request *req) {
  // JC: won't execute read-only if there's a current tentative execution
  // this probably isn't necessary if clients wait for 2f+1 RO responses
  if (last_tentative_execute == last_executed && !state.in_fetch_state() &&
      !state.in_check_state()) {
    // Create a new Reply message.
    Reply *rep = new Reply(view(), req->request_id(), node_id);

    // Obtain "in" and "out" buffers to call exec_command
    Byz_req inb;
    Byz_rep outb;

    inb.contents = req->command(inb.size);
    outb.contents = rep->store_reply(outb.size);

    // Execute command.
    int cid = req->client_id();
    Principal *cp = i_to_p(cid);
    int error = exec_command(&inb, &outb, 0, cid, true);

    if (outb.size % ALIGNMENT_BYTES)
      for (int i = 0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES); i++)
        outb.contents[outb.size + i] = 0;

    if (!error) {
      // Finish constructing the reply and send it.
      rep->authenticate(cp, outb.size, true);
      if (outb.size < 50 || req->replier() == node_id || req->replier() < 0) {
        // Send full reply.
        send(rep, cid);
      } else {
        // Send empty reply.
        Reply empty(view(), req->request_id(), node_id, rep->digest(), cp,
                    true);
        send(&empty, cid);
      }
    }

    delete rep;
    return true;
  } else {
    return false;
  }
}

void Replica::execute_prepared(bool committed) {
  MEMSTATS_MEM_TYPE_VAR
  if (last_tentative_execute < last_executed + 1 &&
      last_executed < last_stable + max_out && !state.in_fetch_state() &&
      !state.in_check_state() && has_new_view()) {
    Pre_prepare *pp = prepared(last_executed + 1);

    if (pp && pp->view() == view()) {
      // Can execute the requests in the message with sequence number
      // last_executed+1.
      last_tentative_execute = last_executed + 1;
      th_assert(pp->seqno() == last_tentative_execute, "Invalid execution");

      // Iterate over the requests in the message, calling execute for
      // each of them.
      Pre_prepare::Requests_iter iter(pp);
      Request req;

      while (iter.get(req)) {
        int cid = req.client_id();
        if (replies.req_id(cid) >= req.request_id()) {
          // Request has already been executed and we have the reply to
          // the request. Resend reply and don't execute request
          // to ensure idempotence.
          replies.send_reply(cid, view(), id());
          continue;
        }

        // Obtain "in" and "out" buffers to call exec_command
        Byz_req inb;
        Byz_rep outb;
        Byz_buffer non_det;
        inb.contents = req.command(inb.size);
        outb.contents = replies.new_reply(cid, outb.size);
        non_det.contents = pp->choices(non_det.size);

        if (is_replica(cid)) {
          // Handle recovery requests, i.e., requests from replicas,
          // differently.  TODO: make more general to allow other types
          // of requests from replicas.
          //	  printf("\n\n\nExecuting recovery request seqno=%qd rep
          // id=%d\n", last_tentative_execute, cid);

          if (inb.size != sizeof(Seqno)) {
            // Invalid recovery request.
            continue;
          }

          // Change keys. TODO: could change key only for recovering replica.
          if (cid != node_id) {
            send_new_key();
          }

          // Store seqno of execution.
          max_rec_n = last_tentative_execute;

          // Reply includes sequence number where request was executed.
          outb.size = sizeof(last_tentative_execute);
          memcpy(outb.contents, &last_tentative_execute, outb.size);
        } else {
          // Execute command in a regular request.
          exec_command(&inb, &outb, &non_det, cid, false);
          if (outb.size % ALIGNMENT_BYTES)
            for (int i = 0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES);
                 i++)
              outb.contents[outb.size + i] = 0;
          // if (last_tentative_execute%100 == 0)
          //   printf("%s - %qd\n",((node_id == primary()) ? "P" : "B"),
          //   last_tentative_execute);
        }

        // Finish constructing the reply.
        replies.end_reply(cid, req.request_id(), outb.size);

        // Send the reply. Replies to recovery requests are only sent
        // when the request is committed.
        if (outb.size != 0 && !is_replica(cid)) {
          if (outb.size < 50 || req.replier() == node_id || req.replier() < 0) {
            // Send full reply.
            replies.send_reply(cid, view(), id(), !committed);
          } else {
            // Send empty reply.
            Reply empty(view(), req.request_id(), node_id, replies.digest(cid),
                        i_to_p(cid), !committed);
            send(&empty, cid);
          }
        }
      }

      if ((last_executed + 1) % checkpoint_interval == 0) {
        MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
        state.checkpoint(last_executed + 1);
        MEMSTATS_RESTORE_MEM_TYPE();
      }
    }
  }
}

void Replica::execute_committed() {
  MEMSTATS_MEM_TYPE_VAR
  if (!state.in_fetch_state() && !state.in_check_state() && has_new_view()) {
    while (1) {
      if (last_executed >= last_stable + max_out || last_executed < last_stable)
        return;

      Pre_prepare *pp = committed(last_executed + 1);

      if (pp && pp->view() == view()) {
        // Tentatively execute last_executed + 1 if needed.
        execute_prepared(true);

        // Can execute the requests in the message with sequence number
        // last_executed+1.
        last_executed = last_executed + 1;
        //	fprintf(stderr, ":):):):):):):):) Set le = %d\n",
        // last_executed);
        th_assert(pp->seqno() == last_executed, "Invalid execution");

        // Execute any buffered read-only requests
        for (Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove()) {
          execute_read_only(m);
          delete m;
        }

        // Iterate over the requests in the message, marking the saved replies
        // as committed (i.e., non-tentative for each of them).
        Pre_prepare::Requests_iter iter(pp);
        Request req;
        while (iter.get(req)) {
          int cid = req.client_id();
          replies.commit_reply(cid);

          if (is_replica(cid)) {
            // Send committed reply to recovery request.
            if (cid != node_id)
              replies.send_reply(cid, view(), id(), false);
            else
              handle(replies.reply(cid)->copy(cid), true);
          }

          // Remove the request from rqueue if present.
          if (rqueue.remove(cid, req.request_id())) vtimer->stop();
        }

        // Send and log Checkpoint message for the new state if needed.
        if (last_executed % checkpoint_interval == 0) {
          Digest d_state;
          MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
          state.digest(last_executed, d_state);
          MEMSTATS_RESTORE_MEM_TYPE();
          Checkpoint *e = new Checkpoint(last_executed, d_state);
          Certificate<Checkpoint> &cc = elog.fetch(last_executed);
          cc.add_mine(e);
          if (cc.is_complete()) {
            mark_stable(last_executed, true);
            //	    fprintf(stderr, "EXEC call MS %qd!!!\n", last_executed);
          }
          //	  else
          //	    fprintf(stderr, "CP exec %qd not yet. ", last_executed);

          send(e, All_replicas);
#ifdef STATIC_LOG_ALLOCATOR
          delete e;
#endif
          //	  printf(">>>>Checkpointing "); d_state.print(); printf("
          //<<<<\n"); fflush(stdout);
        }
      } else {
        // No more requests to execute at this point.
        break;
      }
    }

    if (rqueue.size() > 0) {
      if (primary() == node_id) {
        // Send a pre-prepare with any buffered requests
        send_pre_prepare();
      } else {
        // If I am not the primary and have pending requests restart the
        // timer.
        vtimer->start();
      }
    }
  }
}

void Replica::update_max_rec() {
  // Update max_rec_n to reflect new state.
  bool change_keys = false;
  for (int i = 0; i < num_replicas; i++) {
    if (replies.reply(i)) {
      int len;
      char *buf = replies.reply(i)->reply(len);
      if (len == sizeof(Seqno)) {
        Seqno nr;
        memcpy(&nr, buf, sizeof(Seqno));

        if (nr > max_rec_n) {
          max_rec_n = nr;
          change_keys = true;
        }
      }
    }
  }

  // Change keys if state fetched reflects the execution of a new
  // recovery request.
  if (change_keys) send_new_key();
}

void Replica::new_state(Seqno c) {
  MEMSTATS_MEM_TYPE_VAR
  //  fprintf(stderr, ":n)e:w):s)t:a)t:e) ");
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  if (vi.has_new_view(v) && c >= low_bound) has_nv_state = true;
  MEMSTATS_RESTORE_MEM_TYPE();

  if (c > last_executed) {
    last_executed = last_tentative_execute = c;
    //    fprintf(stderr, ":):):):):):):):) (new_state) Set le = %d\n",
    //    last_executed);
    if (replies.new_state(&rqueue)) vtimer->stop();

    update_max_rec();

    if (c > last_prepared) last_prepared = c;

    if (c > last_stable + max_out) {
      mark_stable(c - max_out, elog.within_range(c - max_out) &&
                                   elog.fetch(c - max_out).mine());
    }

    // Send checkpoint message for checkpoint "c"
    Digest d;
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
    state.digest(c, d);
    MEMSTATS_RESTORE_MEM_TYPE();
    Checkpoint *ck = new Checkpoint(c, d);
    elog.fetch(c).add_mine(ck);
    send(ck, All_replicas);
#ifdef STATIC_LOG_ALLOCATOR
    delete ck;
#endif
  }

  // Check if c is known to be stable.
  int scount = 0;
  for (int i = 0; i < num_replicas; i++) {
    Checkpoint *ck = sset.fetch(i);
    if (ck != 0 && ck->seqno() >= c) {
      th_assert(ck->stable(), "Invalid state");
      scount++;
    }
  }
  if (scount > f()) mark_stable(c, true);

  if (c > seqno) {
    seqno = c;
  }

  // Execute any committed requests
  execute_committed();

  // Execute any buffered read-only requests
  for (Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove()) {
    execute_read_only(m);
    delete m;
  }

  if (rqueue.size() > 0) {
    if (primary() == id()) {
      // Send pre-prepares for any buffered requests
      send_pre_prepare();
    } else
      vtimer->restart();
  }
}

void Replica::mark_stable(Seqno n, bool have_state) {
  MEMSTATS_MEM_TYPE_VAR
  // XXXXXcheck if this should be < or <=

  //  fprintf(stderr, "mark stable n %qd laststable %qd\n", n, last_stable);

  if (n <= last_stable) return;

  last_stable = n;
  if (last_stable > low_bound) {
    low_bound = last_stable;
  }

  if (have_state && last_stable > last_executed) {
    last_executed = last_tentative_execute = last_stable;
    //    fprintf(stderr, ":):):):):):):):) (mark_stable) Set le = %d\n",
    //    last_executed);
    replies.new_state(&rqueue);
    update_max_rec();

    if (last_stable > last_prepared) last_prepared = last_stable;
  }
  //  else
  //    fprintf(stderr, "OH BASE! OH CLU!\n");

  if (last_stable > seqno) seqno = last_stable;

  // fprintf(stderr, "mark_stable: Truncating plog to %lld have_state=%d\n",
  //         last_stable + 1, have_state);
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  plog.truncate(last_stable + 1);
  clog.truncate(last_stable + 1);
#ifdef STATIC_LOG_ALLOCATOR
  agreement_region::truncate(last_stable + 1);
#endif
  MEMSTATS_RESTORE_MEM_TYPE();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  vi.mark_stable(last_stable);
  MEMSTATS_RESTORE_MEM_TYPE();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
  elog.truncate(last_stable);
#ifdef STATIC_LOG_ALLOCATOR
  checkpoint_region::truncate(last_stable);
#endif
  MEMSTATS_RESTORE_MEM_TYPE();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.discard_checkpoint(last_stable, last_executed);
  MEMSTATS_RESTORE_MEM_TYPE();

  if (have_state) {
    // Re-authenticate my checkpoint message to mark it as stable or
    // if I do not have one put one in and make the corresponding
    // certificate complete.
    Checkpoint *c = elog.fetch(last_stable).mine();
    if (c == nullptr) {
      Digest d_state;
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
      state.digest(last_stable, d_state);
      MEMSTATS_RESTORE_MEM_TYPE();
      c = new Checkpoint(last_stable, d_state, true);
      elog.fetch(last_stable).add_mine(c);
      elog.fetch(last_stable).make_complete();
#ifdef STATIC_LOG_ALLOCATOR
      delete c;
#endif
    } else {
      c->re_authenticate(0, true);
    }

    try_end_recovery();
  }

  // Go over sset transfering any checkpoints that are now within
  // my window to elog.
  Seqno new_ls = last_stable;
  for (int i = 0; i < num_replicas; i++) {
    Checkpoint *c = sset.fetch(i);
    if (c != 0) {
      Seqno cn = c->seqno();
      if (cn < last_stable) {
        c = sset.remove(i);
        delete c;
        continue;
      }

      if (cn <= last_stable + max_out) {
        Certificate<Checkpoint> &cs = elog.fetch(cn);
        cs.add(sset.remove(i));
        if (cs.is_complete() && cn > new_ls) new_ls = cn;
      }
    }
  }

  // XXXXXXcheck if this is safe.
  if (new_ls > last_stable) {
    //    fprintf(stderr, "@@@@@@@@@@@@@@@               @@@@@@@@@@@@@@@
    //    @@@@@@@@@@@@@@@\n");
    mark_stable(new_ls, elog.within_range(new_ls) && elog.fetch(new_ls).mine());
  }

  // Try to send any Pre_prepares for any buffered requests.
  if (primary() == id()) send_pre_prepare();
}

void Replica::handle(Data *m) {
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.handle(m);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::handle(Meta_data *m) {
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.handle(m);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::handle(Meta_data_d *m) {
  MEMSTATS_MEM_TYPE_VAR
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.handle(m);
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::handle(Fetch *m) {
  MEMSTATS_MEM_TYPE_VAR
  int mid = m->id();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  if (!state.handle(m, last_stable) && last_new_key != 0) {
    send(last_new_key, mid);
  }
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::send_new_key() {
  MEMSTATS_MEM_TYPE_VAR
  Node::send_new_key();

  // Cleanup messages in incomplete certificates that are
  // authenticated with the old keys.
  int max = last_stable + max_out;
  int min = last_stable + 1;
  for (Seqno n = min; n <= max; n++) {
    if (n % checkpoint_interval == 0) {
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
      elog.fetch(n).mark_stale();
      MEMSTATS_RESTORE_MEM_TYPE();
    }
  }

  if (last_executed > last_stable) min = last_executed + 1;

  for (Seqno n = min; n <= max; n++) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    plog.fetch(n).mark_stale();
    clog.fetch(n).mark_stale();
    MEMSTATS_RESTORE_MEM_TYPE();
  }

  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  vi.mark_stale();
  MEMSTATS_RESTORE_MEM_TYPE();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  state.mark_stale();
  MEMSTATS_RESTORE_MEM_TYPE();
}

void Replica::send_status() {
  MEMSTATS_MEM_TYPE_VAR
  // Check how long ago we sent the last status message.
  Time cur = currentTime();
  if (diffTime(cur, last_status) > 100000) {
    // Only send new status message if last one was sent more
    // than 100 milliseconds ago.
    last_status = cur;

    if (qs) {
      // Retransmit query stable if I am estimating last stable
      qs->re_authenticate();
      send(qs, All_replicas);
      return;
    }

    if (rr) {
      // Retransmit recovery request if I am waiting for one.
      send(rr, All_replicas);
    }

    // If fetching state, resend last fetch message instead of status.
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
    if (state.retrans_fetch(cur)) {
      state.send_fetch(true);
      MEMSTATS_RESTORE_MEM_TYPE();
      return;
    }
    MEMSTATS_RESTORE_MEM_TYPE();

    Status s(v, last_stable, last_executed, has_new_view(),
             vi.has_nv_message(v));

    if (has_new_view()) {
      // Set prepared and committed bitmaps correctly
      int max = last_stable + max_out;
      for (Seqno n = last_executed + 1; n <= max; n++) {
        Prepared_cert &pc = plog.fetch(n);
        if (pc.is_complete() ||
            state.in_check_state()) {  // XXXXXXadded state.in_check_state()
          s.mark_prepared(n);
          if (clog.fetch(n).is_complete() ||
              state.in_check_state()) {  // XXXXXXadded state.in_check_state()
            s.mark_committed(n);
          }
        }
      }
    } else {
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
      vi.set_received_vcs(&s);
      vi.set_missing_pps(&s);
      MEMSTATS_RESTORE_MEM_TYPE();
    }

    // Multicast status to all replicas.
    s.authenticate();
    send(&s, All_replicas);
  }
}

bool Replica::shutdown() {
  MEMSTATS_MEM_TYPE_VAR
  START_CC(shutdown_time);
  vtimer->stop();

  // Rollback to last checkpoint
  if (!state.in_fetch_state()) {
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
    Seqno rc = state.rollback();
    MEMSTATS_RESTORE_MEM_TYPE();
    last_tentative_execute = last_executed = rc;
    //    fprintf(stderr, ":):):):):):):):) shutd Set le = %d\n",
    //    last_executed);
  }

  if (id() == primary()) {
    // Primary sends a view-change before shutting down to avoid
    // delaying client request processing for the view-change timeout
    // period.
    send_view_change();
  }

  char ckpt_name[1024];
  sprintf(ckpt_name, "/tmp/%s_%d", service_name, id());
  FILE *o = fopen(ckpt_name, "w");

  size_t sz = fwrite(&v, sizeof(View), 1, o);
  sz += fwrite(&limbo, sizeof(bool), 1, o);
  sz += fwrite(&has_nv_state, sizeof(bool), 1, o);

  sz += fwrite(&seqno, sizeof(Seqno), 1, o);
  sz += fwrite(&last_stable, sizeof(Seqno), 1, o);
  sz += fwrite(&low_bound, sizeof(Seqno), 1, o);
  sz += fwrite(&last_prepared, sizeof(Seqno), 1, o);
  sz += fwrite(&last_executed, sizeof(Seqno), 1, o);
  sz += fwrite(&last_tentative_execute, sizeof(Seqno), 1, o);

  bool ret = true;
  for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
    ret &= plog.fetch(i).encode(o);

  for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
    ret &= clog.fetch(i).encode(o);

  for (Seqno i = last_stable; i <= last_stable + max_out; i++)
    ret &= elog.fetch(i).encode(o);

  ret &= state.shutdown(o, last_stable);
  ret &= vi.shutdown(o);

  fclose(o);
  STOP_CC(shutdown_time);

  return ret & (sz == 9);
}

bool Replica::restart(FILE *in) {
  START_CC(restart_time);

  bool ret = true;
  size_t sz = fread(&v, sizeof(View), 1, in);
  sz += fread(&limbo, sizeof(bool), 1, in);
  sz += fread(&has_nv_state, sizeof(bool), 1, in);

  limbo = (limbo != 0);
  cur_primary = v % num_replicas;
  if (v < 0 || id() == primary()) {
    ret = false;
    v = 0;
    limbo = false;
    has_nv_state = true;
  }

  sz += fread(&seqno, sizeof(Seqno), 1, in);
  sz += fread(&last_stable, sizeof(Seqno), 1, in);
  sz += fread(&low_bound, sizeof(Seqno), 1, in);
  sz += fread(&last_prepared, sizeof(Seqno), 1, in);
  sz += fread(&last_executed, sizeof(Seqno), 1, in);
  //  fprintf(stderr, ":):):):):):):):) restart read le = %d\n", last_executed);
  sz += fread(&last_tentative_execute, sizeof(Seqno), 1, in);

  ret &= (low_bound >= last_stable) & (last_tentative_execute >= last_executed);
  ret &= last_prepared >= last_tentative_execute;

  if (!ret) {
    //    fprintf(stderr, "Not ret!!! setting le to 0");
    low_bound = last_stable = last_tentative_execute = last_executed =
        last_prepared = 0;
  }

  plog.clear(last_stable + 1);
  for (Seqno i = last_stable + 1; ret && i <= last_stable + max_out; i++)
    ret &= plog.fetch(i).decode(in);

  clog.clear(last_stable + 1);
  for (Seqno i = last_stable + 1; ret && i <= last_stable + max_out; i++)
    ret &= clog.fetch(i).decode(in);

  elog.clear(last_stable);
  for (Seqno i = last_stable; ret && i <= last_stable + max_out; i++)
    ret &= elog.fetch(i).decode(in);

  ret &= state.restart(in, this, last_stable, last_tentative_execute, !ret);
  ret &= vi.restart(in, v, last_stable, !ret);

  STOP_CC(restart_time);

  return ret & (sz == 9);
}

void Replica::recover() {
  corrupt = false;

  char ckpt_name[1024];
  sprintf(ckpt_name, "/tmp/%s_%d", service_name, id());
  FILE *i = fopen(ckpt_name, "r");

  if (i == NULL || !restart(i)) {
    // Replica is faulty; start from initial state.
    fprintf(stderr, "Unable to restart from checkpoint\n");
    corrupt = true;
  }

  // Initialize recovery variables:
  recovering = true;
  vc_recovering = false;
  se.clear();
  delete qs;
  qs = 0;
  rr_reps.clear();
  delete rr;
  rr = 0;
  recovery_point = Seqno_max;
  for (int i = 0; i < num_replicas; i++) rr_views[i] = 0;

  // Change my incoming session keys and zero client's keys.
  START_CC(nk_time);
  send_new_key();

  unsigned zk[Key_size_u];
  bzero(zk, Key_size);
  for (int i = num_replicas; i < num_principals; i++) {
    Principal *p = i_to_p(i);
    p->set_out_key(zk, p->last_tstamp() + 1);
  }
  STOP_CC(nk_time);

  //  printf("Starting estimation procedure\n");
  // Start estimation procedure.
  START_CC(est_time);
  qs = new Query_stable();
  send(qs, All_replicas);

  // Add my own reply-stable message to the estimator.
  Seqno lc = last_executed / checkpoint_interval * checkpoint_interval;
  Reply_stable *rs =
      new Reply_stable(lc, last_prepared, qs->nonce(), i_to_p(id()));
  se.add(rs, true);
}

void Replica::handle(Query_stable *m) {
  if (m->verify()) {
    Seqno lc = last_executed / checkpoint_interval * checkpoint_interval;
    Reply_stable rs(lc, last_prepared, m->nonce(), i_to_p(m->id()));

    // TODO: should put a bound on the rate at which I send these messages.
    send(&rs, m->id());
  } else {
    if (last_new_key != 0) {
      send(last_new_key, m->id());
    }
  }

  delete m;
}

void Replica::enforce_bound(Seqno b) {
  MEMSTATS_MEM_TYPE_VAR
  th_assert(recovering && se.estimate() >= 0, "Invalid state");

  bool correct = !corrupt && last_stable <= b - max_out && seqno <= b &&
                 low_bound <= b && last_prepared <= b &&
                 last_tentative_execute <= b && last_executed <= b &&
                 (last_tentative_execute == last_executed ||
                  last_tentative_execute == last_executed + 1);

  for (Seqno i = b + 1; correct && (i <= plog.max_seqno()); i++) {
    if (!plog.fetch(i).is_empty()) correct = false;
  }

  for (Seqno i = b + 1; correct && (i <= clog.max_seqno()); i++) {
    if (!clog.fetch(i).is_empty()) correct = false;
  }

  for (Seqno i = b + 1; correct && (i <= elog.max_seqno()); i++) {
    if (!elog.fetch(i).is_empty()) correct = false;
  }

  Seqno known_stable = se.low_estimate();
  if (!correct) {
    fprintf(stderr, "Incorrect state setting low bound to %qd\n", known_stable);
    seqno = last_prepared = low_bound = last_stable = known_stable;
    last_tentative_execute = last_executed = 0;
    limbo = false;
    MEMSTATS_SET_MEM_TYPE(MEM_TYPE_CERTIFICATE_LOGS);
    plog.clear(known_stable + 1);
    clog.clear(known_stable + 1);
    elog.clear(known_stable);
    MEMSTATS_RESTORE_MEM_TYPE();
  }

  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_VIEW_INFO);
  correct &= vi.enforce_bound(b, known_stable, !correct);
  MEMSTATS_RESTORE_MEM_TYPE();
  MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
  correct &= state.enforce_bound(b, known_stable, !correct);
  MEMSTATS_RESTORE_MEM_TYPE();
  corrupt = !correct;
}

void Replica::handle(Reply_stable *m) {
  MEMSTATS_MEM_TYPE_VAR
  if (qs && qs->nonce() == m->nonce()) {
    if (se.add(m)) {
      // Done with estimation.
      delete qs;
      qs = 0;
      recovery_point = se.estimate() + max_out;

      enforce_bound(recovery_point);
      STOP_CC(est_time);

      //      printf("sending recovery request\n");
      // Send recovery request.
      START_CC(rr_time);
      rr = new Request(new_rid());

      int len;
      char *buf = rr->store_command(len);
      th_assert(len >= (int)sizeof(recovery_point), "Request is too small");
      memcpy(buf, &recovery_point, sizeof(recovery_point));

      rr->sign(sizeof(recovery_point));
      send(rr, primary());
      STOP_CC(rr_time);

      //      printf("Starting state checking\n");

      // Stop vtimer while fetching state. It is restarted when the fetch ends
      // in new_state.
      vtimer->stop();
      MEMSTATS_SET_MEM_TYPE(MEM_TYPE_STATE_MANAGEMENT);
      state.start_check(last_executed);
      MEMSTATS_RESTORE_MEM_TYPE();

      // Leave multicast group.
      //            printf("XXX Leaving mcast group\n");
      leave_mcast_group();

      rqueue.clear();
      ro_rqueue.clear();
    }
    return;
  }
  delete m;
}

void Replica::enforce_view(View rec_view) {
  th_assert(recovering, "Invalid state");

  if (rec_view >= v || vc_recovering || (limbo && rec_view + 1 == v)) {
    // Replica's view number is reasonable; do nothing.
    return;
  }

  corrupt = true;
  vi.clear();
  v = rec_view - 1;
  send_view_change();
}

void Replica::handle(Reply *m, bool mine) {
  int mid = m->id();
  int mv = m->view();

  if (rr && rr->request_id() == m->request_id() &&
      (mine || !m->is_tentative())) {
    // Only accept recovery request replies that are not tentative.
    bool added = (mine) ? rr_reps.add_mine(m) : rr_reps.add(m);
    if (added) {
#ifdef STATIC_LOG_ALLOCATOR
      delete m;
#endif
      if (rr_views[mid] < mv) rr_views[mid] = mv;

      if (rr_reps.is_complete()) {
        // I have a valid reply to my outstanding recovery request.
        // Update recovery point
        int len;
        const char *rep = rr_reps.cvalue()->reply(len);
        th_assert(len == sizeof(Seqno), "Invalid message");

        Seqno rec_seqno;
        memcpy(&rec_seqno, rep, len);
        Seqno new_rp =
            rec_seqno / checkpoint_interval * checkpoint_interval + max_out;
        if (new_rp > recovery_point) recovery_point = new_rp;

        //	printf("XXX Complete rec reply with seqno %qd
        // rec_point=%qd\n",rec_seqno,  recovery_point);

        // Update view number
        View rec_view = K_max<View>(f() + 1, rr_views, n(), View_max);
        enforce_view(rec_view);

        try_end_recovery();

        delete rr;
        rr = 0;
      }
    }
    return;
  }
  delete m;
}

void Replica::send_null() {
  th_assert(id() == primary(), "Invalid state");

  Seqno max_rec_point = max_out + (max_rec_n + checkpoint_interval - 1) /
                                      checkpoint_interval * checkpoint_interval;

  if (max_rec_n && max_rec_point > last_stable && has_new_view()) {
    if (rqueue.size() == 0 && seqno <= last_executed &&
        seqno + 1 <= max_out + last_stable) {
      // Send null request if there is a recovery in progress and there
      // are no outstanding requests.
      seqno++;
      MEM_STATS_INIT(Replica::send_null, true);
#ifdef PRINT_MEM_STATISTICS
      Req_queue empty(MEM_STATS_GUARD_PUSH(Req_queue));
#else
      Req_queue empty;
#endif
      Pre_prepare *pp = new Pre_prepare(view(), seqno, empty);
      send(pp, All_replicas);
      plog.fetch(seqno).add_mine(pp);
    }
  }
  ntimer->restart();

  // TODO: backups should force view change if primary does not send null
  // requests to allow recoveries to complete.
}

//
// Timeout handlers:
//

void vtimer_handler() {
  th_assert(replica, "replica is not initialized\n");
  if (!replica->delay_vc())
    replica->send_view_change();
  else
    replica->vtimer->restart();
}

void stimer_handler() {
  th_assert(replica, "replica is not initialized\n");
  replica->send_status();

  replica->stimer->restart();
}

void rec_timer_handler() {
  th_assert(replica, "replica is not initialized\n");
  static int rec_count = 0;

  replica->rtimer->restart();

  if (!replica->rec_ready) {
    // Replica is not ready to recover
    return;
  }

#ifdef RECOVERY
  if (replica->n() - 1 - rec_count % replica->n() == replica->id()) {
    // Start recovery:
    INIT_REC_STATS();

    if (replica->recovering) INCR_OP(incomplete_recs);

    printf("* Starting recovery\n");

    // Checkpoint
    replica->shutdown();

    replica->state.simulate_reboot();

    replica->recover();
  } else {
    if (replica->recovering) INCR_OP(rec_overlaps);
  }

#endif

  rec_count++;
}

void ntimer_handler() {
  th_assert(replica, "replica is not initialized\n");

  replica->send_null();
}

bool Replica::has_req(int cid, const Digest &d) {
  Request *req = rqueue.first_client(cid);

  if (req && req->digest() == d) return true;

  return false;
}

void Replica::join_mcast_group() {
  struct ip_mreq req;
  req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
  req.imr_interface.s_addr = INADDR_ANY;
  int error = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&req,
                         sizeof(req));
  if (error < 0) {
    perror("Unable to join group");
    exit(1);
  }
}

void Replica::leave_mcast_group() {
  struct ip_mreq req;
  req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
  req.imr_interface.s_addr = INADDR_ANY;
  int error = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&req,
                         sizeof(req));
  if (error < 0) {
    perror("Unable to join group");
    exit(1);
  }
}

void Replica::try_end_recovery() {
  if (recovering && last_stable >= recovery_point && !state.in_check_state() &&
      rr_reps.is_complete()) {
    // Done with recovery.
    END_REC_STATS();

    //    printf("XXX join mcast group\n");
    join_mcast_group();

    //        printf("Done with recovery\n");
    recovering = false;

    // Execute any buffered read-only requests
    for (Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove()) {
      execute_read_only(m);
      delete m;
    }
  }
}

#ifndef NO_STATE_TRANSLATION
char *Replica::get_cached_obj(int i) { return state.get_cached_obj(i); }
#endif

}  // namespace libbyzea
