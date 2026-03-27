# TinyBFT Protocol Specification

Complete PBFT (Practical Byzantine Fault Tolerance) protocol specification for porting to any language.

## 1. System Model

### Parameters

| Parameter | Symbol | Formula | Description |
|-----------|--------|---------|-------------|
| Total replicas | n | `3f + 1` | Total number of replicas |
| Max faulty | f | `(n-1) / 3` | Maximum Byzantine faulty replicas |
| Quorum | q | `2f + 1` | Messages needed for certificates |

**Examples:**
- f=1, n=4, quorum=3
- f=2, n=7, quorum=5
- f=3, n=10, quorum=7

### Types

```c
typedef int64_t Seqno;      // Sequence number
typedef int64_t View;       // View number
typedef uint64_t Request_id; // Client request identifier
```

### Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `WINDOW_SIZE` | 256 | Max outstanding requests (`max_out`) - 128 on ESP32 |
| `CHECKPOINT_INTERVAL` | 128 | Checkpoint every N sequence numbers |
| `MAX_MESSAGE_SIZE` | 16384 | Maximum message size in bytes |
| `MAX_NUM_REPLICAS` | 32 | Maximum replicas in cluster |
| `MAX_NUM_CLIENTS` | 1 | Maximum concurrent clients |

**Constraint:** `max_out > checkpoint_interval` (otherwise algorithm cannot make progress)

---

## 2. Primary Selection

```
primary(view) = view % num_replicas
```

Replica 0 is primary in view 0, replica 1 in view 1, etc.

---

## 3. Consensus Protocol Flow

### 3.1 Normal Case Operation

```
Client                    Primary (p)              Replicas (r0..rn)
  |                          |                          |
  |---- Request(cmd) ------->|                          |
  |                          |                          |
  |                          |-- Pre_prepare(v,s,d) --->|
  |                          |                          |
  |<-------------------------|---- Prepare(v,s,d) -----|  (each replica)
  |                          |                          |
  |                          |<--- 2f+1 Prepares ----->|
  |                          |                          |
  |                          |---- Commit(v,s) --------|  (each replica)
  |                          |                          |
  |                          |<--- 2f+1 Commits ------>|
  |                          |                          |
  |<--------- Reply ---------|   (execute & reply)      |
```

### 3.2 Step-by-Step Protocol

#### Step 1: Client Request
- Client sends `Request(command, request_id, client_id)` to primary
- Primary validates and assigns sequence number

#### Step 2: Pre-prepare (Primary only)
- Primary assigns sequence number `s` to request
- Computes `digest = SHA-256(request_digests || non_det_choices)`
- Sends `Pre_prepare(view, seqno, digest, requests, non_det_choices)`
- Adds request to request log

#### Step 3: Prepare (All replicas)
- Each replica receives Pre_prepare
- Validates: correct view, valid sequence number, correct digest
- If valid: sends `Prepare(view, seqno, digest, replica_id)` to all
- Adds to prepare certificate log

#### Step 4: Prepare Certificate (Quorum collection)
- Collect Prepare messages for same `(view, seqno, digest)`
- Certificate complete when:
  - Has matching Pre-prepare
  - Has `f+1` matching Prepare messages from distinct replicas
  - All match the same digest

#### Step 5: Commit
- When prepare certificate is complete: sends `Commit(view, seqno, replica_id)` to all
- Adds to commit certificate log

#### Step 6: Execute
- Commit certificate complete when `2f+1` matching Commit messages collected
- Execute request in order of sequence number
- Send `Reply(view, request_id, digest, result)` to client
- Client accepts reply when `f+1` matching replies received

---

## 4. Sequence Number Management

### 4.1 Window Bounds

Messages are only accepted if within the current window:

```
last_stable = sequence number of last stable checkpoint
window_start = last_stable + 1
window_end = last_stable + max_out

Valid seqno: last_stable < seqno <= last_stable + max_out
```

### 4.2 Sequence Number Assignment (Primary only)

```python
seqno = last_executed + 1  # Next available sequence number
# Advance until finding unused slot in plog
while plog.fetch(seqno) != null:
    seqno += 1
```

### 4.3 Execution Order

Requests execute in strict sequence number order:
```python
while last_executed < last_prepared:
    next = last_executed + 1
    if plog.is_complete(next) and clog.is_complete(next):
        execute(plog[next].request)
        last_executed = next
        if last_executed % checkpoint_interval == 0:
            send_checkpoint()
```

---

## 5. Checkpointing

### 5.1 Checkpoint Generation

Every `checkpoint_interval` sequence numbers:
1. Take checkpoint of application state
2. Compute `digest = SHA-256(state)`
3. Send `Checkpoint(seqno, digest, replica_id)` to all replicas

### 5.2 Checkpoint Stability

Checkpoint at sequence number `s` becomes stable when:
- `2f+1` matching `Checkpoint(s, digest)` messages collected
- All have same digest

### 5.3 Garbage Collection

When checkpoint becomes stable:
1. Set `last_stable = s`
2. Discard all log entries with `seqno <= s`
3. Window slides forward: `[last_stable+1, last_stable+max_out]`

### 5.4 State Transfer

If more than `f` stable checkpoints exist above the window:
- Initiate state transfer to catch up
- Fetch state from replicas with matching checkpoint digests

---

## 6. View Change

### 6.1 Trigger

View change timer expires (primary not sending Pre-prepare messages within timeout).

**Timeout values (from config):**
- View change timeout: 10000ms (default)
- Status timeout: 1000ms
- Recovery timeout: 60000ms

### 6.2 View Change Process

```
1. Timer expires
2. Increment view: v = v + 1
3. cur_primary = v % n
4. Rollback to last stable checkpoint if needed
5. Clear prepare/commit logs above last_stable
6. Send View_change(v, last_stable, checkpoints, prepared_requests)
7. Wait for 2f+1 View_change messages for view v
8. New primary sends New_view with re-proposed requests
```

### 6.3 View Change Message Contents

```c
struct View_change_rep {
    View v;                    // New view number
    Seqno ls;                  // Last stable checkpoint seqno
    Digest ckpts[];            // Checkpoint digests (array)
    int id;                    // Sending replica ID
    short n_ckpts;             // Number of checkpoint entries
    short n_reqs;              // Number of request entries
    uint64_t prepared[];       // Bitmap of prepared requests
    Digest d;                  // Digest of entire message
    Req_info req_info[];       // Request information
};
```

### 6.4 Req_info Structure

```c
struct Req_info {
    View lv;    // 8 bytes - Last view where pre-prepare/prepare sent
    View v;     // 8 bytes - View where request prepared
    Digest d;   // 32 bytes - Request digest (SHA-256)
};
```

### 6.5 New View Processing

New primary (view % n) sends `New_view` containing:
1. View number
2. Min/max sequence number range
3. Missing Pre-prepare messages for prepared requests
4. Proof that 2f+1 replicas sent View_change

---

## 7. Request Deduplication

### 7.1 Client-Side
- Each request has unique `(client_id, request_id)` pair
- Client increments `request_id` for each new request
- Client retransmits with same `request_id` if no reply received

### 7.2 Replica-Side
- Track `last_rid[client_id]` for each client
- If `request_id <= last_rid[client_id]`: retransmit cached reply
- If `request_id > last_rid[client_id]`: process new request

---

## 8. Key Rotation

### 8.1 New_key Message
- Sent periodically (every `auth_timeout` milliseconds)
- Contains fresh 32-byte HMAC session keys for each peer
- Encrypted with each peer's RSA public key (RSA-OAEP)
- Signed with sender's RSA private key (RSA-PSS)

### 8.2 After Key Rotation
- All existing certificates marked as stale
- New authenticators use fresh keys
- Old messages with stale authenticators are rejected

---

## 9. State Machine Summary

```
States:
  INITIALIZED -> RUNNING -> (VIEW_CHANGE) -> RUNNING
  
Sub-states within RUNNING:
  RECEIVED_PRE_PREPARE -> PREPARED -> COMMITTED -> EXECUTED
  
View change:
  RUNNING --timer_expired--> VIEW_CHANGING --2f+1_vc--> NEW_VIEW --> RUNNING
```

### State Variables (per replica)

```c
Seqno seqno;                    // Next seqno (primary only)
Seqno last_stable;              // Last stable checkpoint
Seqno low_bound;                // Low bound during view change
Seqno last_prepared;            // Highest prepared
Seqno last_executed;            // Highest executed
Seqno last_tentative_execute;   // Highest tentatively executed
View  v;                        // Current view
int   cur_primary;              // v % n
bool  limbo;                    // In view change
```

---

## 10. Safety Properties

1. **Agreement:** No two correct replicas execute different requests at the same sequence number
2. **Total order:** All correct replicas execute requests in the same order
3. **Validity:** If a correct client sends a request, it is eventually executed
4. **Liveness:** The system makes progress if at most f replicas are faulty (requires eventual synchrony)

---

## Source Files

- `src/Replica.cc` - Main consensus loop and message handlers
- `src/Pre_prepare.cc` - Pre-prepare message creation
- `src/Prepare.cc` - Prepare message handling
- `src/Commit.cc` - Commit certificate management
- `src/View_change.cc` - View change protocol
- `src/Checkpoint.cc` - Checkpointing logic
- `src/Certificate.t` - Quorum computation templates
- `src/Prepared_cert.cc` - Prepare certificate logic
- `include/types.h` - Basic types
- `src/parameters.h` - Protocol constants
