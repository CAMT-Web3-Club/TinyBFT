# TinyBFT Implementation Checklist

Step-by-step checklist for porting TinyBFT to another language.

## Phase 1: Foundation

### 1.1 Basic Types

- [ ] `Seqno` (int64) - Sequence number
- [ ] `View` (int64) - View number
- [ ] `Request_id` (uint64) - Client request ID
- [ ] `Digest` (32 bytes) - SHA-256 hash
- [ ] Message header (8 bytes: tag:int16, extra:int16, size:int32)
- [ ] 8-byte alignment for all message sizes

### 1.2 Cryptography

- [ ] SHA-256 digest (32-byte output)
- [ ] Constant-time digest comparison
- [ ] HMAC-SHA256 (32-byte key, 32-byte MAC)
- [ ] Constant-time MAC comparison
- [ ] RSA-1024 key loading from PEM files
- [ ] RSA-PSS-SHA256 signature generation
- [ ] RSA-PSS-SHA256 signature verification
- [ ] RSA-OAEP-SHA256 encryption (for 32-byte keys)
- [ ] RSA-OAEP-SHA256 decryption
- [ ] CSPRNG (cryptographically secure random number generator)

### 1.3 Configuration

- [ ] Config file parser (sequential fscanf format)
- [ ] Parse: service_name, max_faulty, auth_timeout, num_principals
- [ ] Parse: multicast_addr, port
- [ ] Parse: hostname, ip, port, keyfile per principal
- [ ] Parse: view_change_timeout, status_timeout, recovery_timeout
- [ ] Derive: n=3f+1, quorum=2f+1, primary=view%n

---

## Phase 2: Message Types

### 2.1 Request (tag=1)

- [ ] `Request_rep` structure (od:Digest, replier:int16, command_size:int16, cid:int32, rid:uint64)
- [ ] Create request with command
- [ ] Sign request (RSA-PSS)
- [ ] Verify request signature
- [ ] Extract command from request
- [ ] Compute request digest (SHA-256 of rid||cid||command)

### 2.2 Reply (tag=2)

- [ ] `Reply_rep` structure (v:View, rid:uint64, digest:Digest, replica:int32, reply_size:int32)
- [ ] Create reply with result
- [ ] Authenticate reply (HMAC)
- [ ] Verify reply authenticator
- [ ] Check tentative flag (extra != 0)

### 2.3 Pre-prepare (tag=3)

- [ ] `Pre_prepare_rep` structure (v:View, s:Seqno, digest:Digest, rset_size:int32, non_det_size:int16)
- [ ] Create pre-prepare with requests
- [ ] Compute digest (SHA-256 of request_set || non_det_choices)
- [ ] Sign pre-prepare
- [ ] Verify pre-prepare (signature + digest + request authenticity)
- [ ] Iterate requests in pre-prepare

### 2.4 Prepare (tag=4)

- [ ] `Prepare_rep` structure (v:View, s:Seqno, digest:Digest, id:int32, padding:int32)
- [ ] Create prepare message
- [ ] Authenticate prepare (HMAC or RSA)
- [ ] Verify prepare
- [ ] Match check: same (view, seqno, digest)

### 2.5 Commit (tag=5)

- [ ] `Commit_rep` structure (v:View, s:Seqno, id:int32, padding:int32)
- [ ] Create commit message
- [ ] Authenticate commit (HMAC)
- [ ] Verify commit
- [ ] Match check: same (view, seqno)

### 2.6 Checkpoint (tag=6)

- [ ] `Checkpoint_rep` structure (s:Seqno, digest:Digest, id:int32, padding:int32)
- [ ] Create checkpoint with state digest
- [ ] Authenticate checkpoint
- [ ] Verify checkpoint
- [ ] Check stable flag (extra == 1)

### 2.7 View Change (tag=8)

- [ ] `View_change_rep` structure (v:View, ls:Seqno, ckpts[], id:int32, n_ckpts:int16, n_reqs:int16, prepared[], d:Digest)
- [ ] `Req_info` structure (lv:View, v:View, d:Digest)
- [ ] Create view change with checkpoints and requests
- [ ] Add checkpoint digest
- [ ] Add request info (prepared/unprepared)
- [ ] Compute message digest
- [ ] Sign view change
- [ ] Verify view change (signature + digest)

### 2.8 New View (tag=9)

- [ ] `New_view_rep` structure (v:View, min:Seqno, max:Seqno)
- [ ] Create new view with proofs
- [ ] Sign new view
- [ ] Verify new view

### 2.9 New Key (tag=11)

- [ ] Create new key message with encrypted session keys
- [ ] Encrypt keys with RSA-OAEP for each peer
- [ ] Sign new key message
- [ ] Verify new key signature
- [ ] Decrypt session keys
- [ ] Update session keys

---

## Phase 3: Certificate Collection

### 3.1 Generic Certificate

- [ ] Store up to n messages for a (view, seqno) pair
- [ ] Count distinct values
- [ ] Track "correct" value (f+1 matching = correct)
- [ ] Check "complete" (2f+1 matching = quorum)
- [ ] Constant-time value comparison

### 3.2 Prepare Certificate

- [ ] Store Pre-prepare + up to n Prepare messages
- [ ] Check: has pre-prepare AND f+1 matching prepares
- [ ] Check: pre-prepare digest matches prepare digest
- [ ] Mark certificate as prepared

### 3.3 Commit Certificate

- [ ] Store up to n Commit messages
- [ ] Check: 2f+1 matching commits for same (view, seqno)
- [ ] Mark certificate as committed

### 3.4 Checkpoint Certificate

- [ ] Store up to n Checkpoint messages
- [ ] Check: 2f+1 matching checkpoints for same (seqno, digest)
- [ ] Mark checkpoint as stable

---

## Phase 4: Request Management

### 4.1 Request Queue

- [ ] FIFO queue for pending requests
- [ ] Deduplication by (client_id, request_id)
- [ ] Remove requests included in pre-prepare
- [ ] Read-only request handling

### 4.2 Reply Cache

- [ ] Store last reply per client
- [ ] Track last request_id per client
- [ ] Retransmit cached reply for duplicate requests
- [ ] Discard old requests (request_id <= last_rid)

---

## Phase 5: Transport Layer

### 5.1 Transport Interface

- [ ] `init()` - Initialize network/hardware
- [ ] `send(message, dest_id)` - Send to peer or all
- [ ] `recv()` - Non-blocking receive (returns null if empty)
- [ ] `add_peer(id, addr)` - Register peer
- [ ] `is_ready()` - Check initialization status
- [ ] `max_payload_size()` - Return max single-frame payload

### 5.2 UDP Transport (Linux)

- [ ] Create UDP socket (DGRAM, non-blocking)
- [ ] Join multicast group
- [ ] Send to multicast group for All_replicas
- [ ] Send to unicast for specific peer
- [ ] Unicast fallback when multicast disabled
- [ ] Parse peer addresses from config

### 5.3 ESP-NOW Transport (ESP32)

- [ ] Initialize WiFi (STA mode)
- [ ] Set WiFi channel
- [ ] Initialize ESP-NOW
- [ ] Register send/receive callbacks
- [ ] Add peers by MAC address
- [ ] Fragment messages > 1470 bytes
- [ ] Reassemble fragmented messages
- [ ] Load peer MACs from SPIFFS config

### 5.4 Fragmentation

- [ ] Fragment header (16 bytes: msg_id, seq, total, size, dest_id, protocol, flags, padding)
- [ ] Split message into 1454-byte payloads
- [ ] Reassemble by msg_id
- [ ] Timeout for incomplete reassembly (5000ms)

---

## Phase 6: Replica Logic

### 6.1 Initialization

- [ ] Load config file
- [ ] Load private key
- [ ] Initialize transport
- [ ] Initialize certificate logs (size = WINDOW_SIZE)
- [ ] Initialize timers (view_change, status, recovery, auth)
- [ ] Set initial view = 0, primary = 0

### 6.2 Request Handling (Primary)

- [ ] Receive client request
- [ ] Validate request signature
- [ ] Add to request queue
- [ ] Assign sequence number
- [ ] Create Pre-prepare with requests
- [ ] Sign and broadcast Pre-prepare

### 6.3 Pre-prepare Handling (All Replicas)

- [ ] Validate view number
- [ ] Validate sequence number (within window)
- [ ] Verify signature and digest
- [ ] Store in prepare log
- [ ] Send Prepare to all replicas

### 6.4 Prepare Handling

- [ ] Validate message
- [ ] Add to prepare certificate
- [ ] If certificate complete: send Commit

### 6.5 Commit Handling

- [ ] Validate message
- [ ] Add to commit certificate
- [ ] If certificate complete: execute request

### 6.6 Execution

- [ ] Execute requests in sequence number order
- [ ] Last executed < last committed
- [ ] Call application callback
- [ ] Send Reply to client
- [ ] Update last_executed
- [ ] Take checkpoint if at interval

### 6.7 Checkpoint Handling

- [ ] Receive checkpoint messages
- [ ] Add to checkpoint certificate
- [ ] If 2f+1 matching: mark stable
- [ ] Truncate logs below stable checkpoint
- [ ] Advance window

---

## Phase 7: View Change

### 7.1 View Change Trigger

- [ ] View change timer expires
- [ ] Increment view number
- [ ] Update primary (view % n)
- [ ] Rollback to last stable checkpoint if needed
- [ ] Clear logs above last_stable
- [ ] Build and send View_change message

### 7.2 View Change Reception

- [ ] Receive View_change from peer
- [ ] Validate signature
- [ ] If f+1 for higher view: trigger own view change
- [ ] Store in view change certificate

### 7.3 New View Processing

- [ ] New primary collects 2f+1 View_change messages
- [ ] Build New_view with re-proposed pre-prepares
- [ ] Send New_view to all
- [ ] Replicas verify New_view proofs
- [ ] Resume normal operation

---

## Phase 8: Key Management

### 8.1 Key Generation

- [ ] Generate RSA-1024 private key
- [ ] Extract public key
- [ ] Generate 32-byte HMAC session keys

### 8.2 Key Distribution

- [ ] Periodic New_key message (auth_timeout interval)
- [ ] Encrypt session keys with peer's RSA public key
- [ ] Sign New_key with own private key
- [ ] Broadcast to all

### 8.3 Key Update

- [ ] Verify New_key signature
- [ ] Decrypt session keys
- [ ] Update kin/kout for each peer
- [ ] Re-authenticate pending certificates
- [ ] Mark old certificates as stale

---

## Phase 9: Testing

### 9.1 Unit Tests

- [ ] SHA-256 produces correct digests
- [ ] HMAC-SHA256 produces correct MACs
- [ ] RSA sign/verify works
- [ ] RSA-OAEP encrypt/decrypt works
- [ ] Message serialization/deserialization
- [ ] Certificate collection (quorum counting)
- [ ] Request deduplication

### 9.2 Integration Tests

- [ ] Single request consensus (4 replicas)
- [ ] Multiple request ordering
- [ ] View change on primary failure
- [ ] Fault tolerance (survive f failures)
- [ ] 7-replica test suite

### 9.3 ESP32 Tests

- [ ] ESP-NOW peer discovery
- [ ] Fragmentation/reassembly
- [ ] Memory usage within budget
- [ ] Timer accuracy
- [ ] Consensus on hardware

---

## Phase 10: Optimization

### 10.1 Performance

- [ ] Batch requests in Pre-prepare
- [ ] Parallel crypto operations (if platform supports)
- [ ] Efficient certificate lookup (hash map)
- [ ] Message reuse (avoid allocation)

### 10.2 Memory

- [ ] Static allocation for embedded
- [ ] Message pool (reuse allocated messages)
- [ ] Compact certificate storage
- [ ] Configurable window size

### 10.3 Network

- [ ] Nagle's algorithm disabled (TCP_NODELAY for UDP reliability layer)
- [ ] Appropriate send/receive buffer sizes
- [ ] Retransmission for lost messages

---

## Verification

After implementation, verify against reference:

| Test | Expected Result |
|------|-----------------|
| Config parsing | n=7, f=2, quorum=5 |
| Request digest | 32-byte SHA-256 |
| HMAC authenticator | 32 bytes per peer |
| RSA signature | 132 bytes (4+len+128) |
| Pre-prepare digest | Matches request set hash |
| Quorum check | 2f+1 = 5 for f=2 |
| View change | New primary after timeout |
| Checkpoint | Stable at 2f+1 matching |

---

## Reference Implementation

The C++ implementation in `src/` serves as the reference. Key files to study:

1. `src/Replica.cc` - Main consensus logic (1200+ lines)
2. `src/Node.cc` - Base node, config parsing (600+ lines)
3. `src/Client.cc` - Client request handling
4. `src/Certificate.t` - Quorum computation template
5. `src/EspNowTransport.cc` - ESP-NOW integration
6. `src/Fragmentation.cc` - Message fragmentation
