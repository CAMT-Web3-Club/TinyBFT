# TinyBFT Wire Protocol Specification

Exact byte-level message format for interoperability between implementations.

## 1. Alignment

All messages MUST be 8-byte aligned. Message `size` field must be a multiple of 8.

```
ALIGNMENT = 8
ALIGNED_SIZE(x) = ((x + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT
```

---

## 2. Base Message Header

Every message starts with this 8-byte header:

```
Offset  Size  Type    Field   Description
------  ----  ------  ------  -----------
0       2     int16   tag     Message type (see tags below)
2       2     int16   extra   Extra flags (message-specific)
4       4     int32   size    Total message size (must be 8-byte aligned)
```

### Message Tags

```c
Free_message_tag     = 0   // Unused slot
Request_tag          = 1   // Client request
Reply_tag            = 2   // Replica reply
Pre_prepare_tag      = 3   // Primary pre-prepare
Prepare_tag          = 4   // Replica prepare
Commit_tag           = 5   // Replica commit
Checkpoint_tag       = 6   // Checkpoint
Status_tag           = 7   // Status sync
View_change_tag      = 8   // View change
New_view_tag         = 9   // New view announcement
View_change_ack_tag  = 10  // View change ack
New_key_tag          = 11  // Key rotation
Meta_data_tag        = 12  // State transfer metadata
Meta_data_d_tag      = 13  // State transfer metadata digest
Data_tag             = 14  // State transfer data
Fetch_tag            = 15  // State transfer fetch
Query_stable_tag     = 16  // Query stable checkpoint
Reply_stable_tag     = 17  // Reply stable checkpoint
```

---

## 3. Request (tag=1)

Client sends request to primary. Signed with RSA-PSS.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=1, size=total
8       32      Digest    od             SHA-256(rid || cid || command)
40      2       int16     replier        Expected replier (-1 = all)
42      2       int16     command_size   Command length in bytes
44      4       int32     cid            Client ID
48      8       uint64    rid            Request ID (unique per client)
56      N       bytes     command        Command payload
56+N    M       bytes     signature      RSA-PSS signature
```

**Flags (extra field):**
- Bit 0: read-only flag
- Bit 2: recovery request flag

**Deduplication key:** `(cid, rid)` pair

---

## 4. Reply (tag=2)

Replica sends reply to client. Authenticated with HMAC.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=2, size=total
8       8       View      v              Current view number
16      8       RequestID rid            Request ID
24      32      Digest    digest         Reply digest
56      4       int32     replica        Sending replica ID
60      4       int32     reply_size     Reply length (negative = empty)
64      N       bytes     reply          Reply payload
64+N    32      bytes     mac            HMAC-SHA256 authenticator
```

**Flags (extra field):**
- `extra != 0`: tentative reply
- `extra == 0`: committed reply

**Reply validation:** Client accepts when `f+1` matching replies received.

---

## 5. Pre-prepare (tag=3)

Primary assigns sequence number to requests.

```
Offset  Size      Type      Field          Description
------  --------  --------  -------------  -----------
0       8         header    (base header)  tag=3, size=total
8       8         View      v              View number
16      8         Seqno     seqno          Sequence number
24      32        Digest    digest         SHA-256(rset || non_det)
56      4         int32     rset_size      Request set size (bytes)
60      2         int16     non_det_size   Non-det choices size (bytes)
62      2         padding   (unused)       Alignment padding
64      rset_size bytes     requests       Serialized request set
64+rs   nd_size   bytes     non_det        Non-deterministic choices
64+rs+  M         bytes     signature      RSA-PSS signature
  nd
64+rs+  AUTH_SIZE bytes     authenticator  HMAC array
  nd+M
```

**Digest computation:**
```python
digest = SHA-256(request_digests_concatenated || non_det_choices)
```

**Request set format:** Serialized sequence of Request messages (each 8-byte aligned).

---

## 6. Prepare (tag=4)

Replica votes on Pre-prepare.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=4, size=total
8       8       View      v              View number
16      8       Seqno     seqno          Sequence number
24      32      Digest    digest         Must match Pre-prepare digest
56      4       int32     id             Sending replica ID
60      4       int32     padding        Alignment
64      M       bytes     signature      RSA-PSS signature
64+M    AUTH_SZ bytes     authenticator  HMAC array
```

**Matching:** Prepare matches Pre-prepare if same `(view, seqno, digest)`.

**Quorum:** Need `f+1` matching Prepares + matching Pre-prepare = prepared certificate.

---

## 7. Commit (tag=5)

Replica commits to execute request.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=5, size=total
8       8       View      v              View number
16      8       Seqno     seqno          Sequence number
24      4       int32     id             Sending replica ID
28      4       int32     padding        Alignment
32      AUTH_SZ bytes     authenticator  HMAC array
```

**Matching:** Commits match if same `(view, seqno)`.

**Quorum:** Need `2f+1` matching Commits = committed certificate.

---

## 8. Checkpoint (tag=6)

Replica announces stable state at sequence number.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=6, size=total
8       8       Seqno     seqno          Sequence number
16      32      Digest    digest         State digest at seqno
48      4       int32     id             Sending replica ID
52      4       int32     padding        Alignment
56      M       bytes     signature      RSA-PSS signature
56+M    AUTH_SZ bytes     authenticator  HMAC array
```

**Flags (extra field):**
- `extra == 1`: checkpoint is known to be stable
- `extra == 0`: checkpoint is tentative

**Stability:** `2f+1` matching Checkpoints = stable checkpoint.

---

## 9. View Change (tag=8)

Replica requests view change.

```
Offset  Size          Type         Field          Description
------  ------------  -----------  -------------  -----------
0       8             header       (base header)  tag=8, size=total
8       8             View         v              New view number
16      8             Seqno        ls             Last stable checkpoint
24      32*N          Digest[]     ckpts[]        Checkpoint digests (N=max_out/ckpt_int+1)
var     4             int32        id             Sending replica ID
var+4   2             int16        n_ckpts        Number of ckpts
var+6   2             int16        n_reqs         Number of requests
var+8   8*K           uint64[]     prepared[]     Bitmap of prepared reqs (K=max_out/64)
var+8+  32            Digest       d              Message digest
  8*K
var     variable      Req_info[]   req_info[]     Request info entries (n_reqs entries)
var     M             bytes        signature      RSA-PSS signature
var+M   AUTH_SZ       bytes        authenticator  HMAC array
```

**Req_info structure (48 bytes each):**
```c
struct Req_info {
    View lv;    // 8 bytes - Last view where pre-prepare/prepare sent
    View v;     // 8 bytes - View where request prepared
    Digest d;   // 32 bytes - Request digest
};
```

**prepared bitmap:** Bit i is set if request with `seqno = ls + i + 1` is prepared.

---

## 10. New View (tag=9)

New primary announces view change completion.

```
Offset  Size      Type         Field          Description
------  --------  -----------  -------------  -----------
0       8         header       (base header)  tag=9, size=total
8       8         View         v              New view number
16      8         Seqno        min            Checkpoint seqno to propagate
24      8         Seqno        max            Requests < max propagated
32      variable  VC_info[]    vc_info        View change proofs (f+1 entries)
var     variable  Pre_prepare[] picked         Re-proposed pre-prepares
var     M         bytes        signature      RSA-PSS signature
var+M   AUTH_SZ   bytes        authenticator  HMAC array
```

---

## 11. New Key (tag=11)

Key rotation message. Signed with RSA-PSS, keys encrypted with RSA-OAEP.

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       8       header    (base header)  tag=11, size=total
8       4       int32     id             Sending replica ID
12      4       int32     count          Number of key entries
16      N*entry bytes     keys[]         Key entries
16+N*M  M       bytes     signature      RSA-PSS signature
```

**Key entry format (per peer):**
```
Offset  Size    Type      Field          Description
0       4       int32     peer_id        Target peer ID
4       4       int32     key_size       Encrypted key size
8       KS      bytes     enc_key        RSA-OAEP encrypted 32-byte key
```

---

## 12. Status (tag=7)

Replica announces its current state and missing information.

```
Offset  Size      Type         Field          Description
------  --------  -----------  -------------  -----------
0       8         header       (base header)  tag=7, size=total
8       8         View         v              Replica's current view
16      8         Seqno        ls             Seqno of last stable checkpoint
24      8         Seqno        le             Seqno of last request executed
32      4         int32        id             Replica ID
36      2         int16        sz             Size of bitmaps/arrays
38      2         int16        brsz           Size of big request info
40      variable  -            payload        Prepared/Committed bitmaps or VC/PP info
var     AUTH_SZ   bytes        authenticator  HMAC array
```

---

## 13. Authenticator Format

### HMAC Mode (default)

```
Size: (num_replicas - 1) * 32 bytes

Layout:
  MAC_0: HMAC-SHA256(session_key, message_header) [32 bytes]
  MAC_1: HMAC-SHA256(session_key, message_header) [32 bytes]
  ...
  MAC_n: HMAC-SHA256(session_key, message_header) [32 bytes]
  (skipping sender's own MAC)

Verification offset:
  offset = (my_id < sender_id) ? my_id : my_id - 1
  verify MAC at offset using sender's session key
```

### RSA Mode (PKEY flag)

```
Size: 4 + key_size bytes (typically 132 bytes for 1024-bit RSA)

Layout:
  4 bytes:    signature length (key_size)
  N bytes:    RSA-PSS-SHA256 signature of message digest
```

---

## 13. Fragmentation (ESP-NOW only)

When message exceeds ESP-NOW MTU (1470 bytes), split into fragments.

### Fragment Header (16 bytes)

```
Offset  Size    Type      Field          Description
------  ------  --------  -------------  -----------
0       4       uint32    msg_id         Unique message ID (auto-increment)
4       2       uint16    seq            Fragment sequence (0, 1, 2, ...)
6       2       uint16    total          Total fragment count
8       2       uint16    size           Data size in this fragment
10      2       uint16    dest_id        Destination replica ID
12      1       uint8     protocol       Protocol version (1)
13      1       uint8     flags          0x01 = last fragment
14      2       uint16    padding        Alignment
```

### Fragment Payload

```
Max payload per fragment: 1470 - 16 = 1454 bytes

Example: 8192-byte message
  Total fragments: ceil(8192 / 1454) = 6
  Frag 0: header + bytes [0..1453]
  Frag 1: header + bytes [1454..2907]
  ...
  Frag 5: header + bytes [7270..8191] (flags=0x01)
```

### Reassembly

1. Parse fragment header
2. Verify `protocol == 1`
3. Store in pending map by `msg_id`
4. When all fragments received (or timeout 5000ms): concatenate and return
5. Single-fragment messages (`total == 0` or `total == 1`): pass through

---

## 14. Complete Message Size Formulas

### Request
```
sizeof(Request_rep) = 8 + 32 + 2 + 2 + 4 + 8 = 56 bytes
total = 56 + command_size + signature_size + authenticator_size
```

### Pre-prepare
```
sizeof(Pre_prepare_rep) = 8 + 8 + 8 + 32 + 4 + 2 + 2 = 64 bytes (aligned)
total = 64 + rset_size + non_det_size + signature_size + authenticator_size
```

### Prepare
```
sizeof(Prepare_rep) = 8 + 8 + 8 + 32 + 4 + 4 = 64 bytes
total = 64 + signature_size + authenticator_size
```

### Commit
```
sizeof(Commit_rep) = 8 + 8 + 8 + 4 + 4 = 32 bytes
total = 32 + authenticator_size
```

### Checkpoint
```
sizeof(Checkpoint_rep) = 8 + 8 + 32 + 4 + 4 = 56 bytes
total = 56 + signature_size + authenticator_size
```

### Reply
```
sizeof(Reply_rep) = 8 + 8 + 8 + 32 + 4 + 4 = 64 bytes
total = 64 + reply_size + mac_size (32 bytes)
```

### Status
```
sizeof(Status_rep) = 8 + 8 + 8 + 8 + 4 + 2 + 2 = 40 bytes
total = 40 + payload_size + authenticator_size
```

### Authenticator size
```
HMAC mode: AUTHENTICATOR_SIZE = Digest::SIZE * (MAX_NUM_REPLICAS - 1)
           = 32 * (n - 1)
RSA mode:  AUTHENTICATOR_SIZE = 256 (fixed)
```

---

## Source Files

- `src/Message.h` - Base message header (`Message_rep`)
- `src/Message_tags.h` - Tag constants
- `src/Request.h` - Request structure
- `src/Reply.h` - Reply structure
- `src/Pre_prepare.h` - Pre-prepare structure
- `src/Prepare.h` - Prepare structure
- `src/Commit.h` - Commit structure
- `src/Checkpoint.h` - Checkpoint structure
- `src/View_change.h` - View change structure
- `src/Fragmentation.h` - Fragment header
- `src/Digest.h` - Digest type (32-byte SHA-256)
