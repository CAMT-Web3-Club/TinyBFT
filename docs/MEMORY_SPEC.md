# TinyBFT Memory Specification

Memory management, allocation strategies, and resource budgets for porting.

## 1. Memory Allocation Modes

TinyBFT supports two allocation strategies:

### 1.1 Dynamic Mode (Linux default)

```c
// Not defined: uses Log_allocator for messages
// Falls back to malloc/free for other objects
```

- Messages allocated via `Log_allocator` (block allocator)
- Default chunk size: 65536 bytes
- Other objects: standard `new`/`delete`/`malloc`/`free`

### 1.2 Static Mode (ESP32 default)

```c
#define STATIC_LOG_ALLOCATOR
```

- All memory statically pre-allocated
- No heap fragmentation
- Four memory regions:
  - `agreement_region` - Consensus messages
  - `checkpoint_region` - Checkpoint data
  - `special_region` - Persistent state (keys, replies)
  - `scratch_allocator` - Temporary buffers

---

## 2. Buffer Size Formulas

### 2.1 Message Buffers

| Buffer | Formula | Default (Linux) | Default (ESP32) |
|--------|---------|-----------------|-----------------|
| Max message | `MAX_MESSAGE_SIZE` | 16384 | 8192 |
| Max payload (UDP) | 65507 | 65507 | N/A |
| Max payload (ESP-NOW) | `1470 - 16` | N/A | 1454 |
| Max fragment | 1470 | N/A | 1470 |

### 2.2 Request Size

```c
max_request_size = ALIGNED_SIZE(
    MAX_REQUEST_SIZE + sizeof(Request_rep) + sizeof(uint32_t) + 256
)
// where:
MAX_REQUEST_SIZE = MAX_MESSAGE_SIZE - sizeof(Pre_prepare_rep)
                  - AUTHENTICATOR_SIZE - sizeof(Request_rep) - 256 - 4
```

### 2.3 View Change Size

```c
max_view_change_size = sizeof(View_change_rep)
                     + WINDOW_SIZE * sizeof(Req_info)
                     + AUTHENTICATOR_SIZE
// sizeof(Req_info) = 48 bytes
```

**Constraint:** `max_view_change_size <= MAX_MESSAGE_SIZE`

### 2.4 Authenticator Size

```c
// HMAC mode
AUTHENTICATOR_SIZE = Digest::SIZE * (MAX_NUM_REPLICAS - 1)
                   = 32 * (n - 1)

// RSA mode (PKEY)
AUTHENTICATOR_SIZE = 256  // Fixed for 2048-bit RSA max
```

---

## 3. Certificate Log Sizes

Each log is sized for `WINDOW_SIZE` (max_out) entries:

### 3.1 Prepare Log (`plog`)

```c
Log<Prepared_cert> plog;  // Size: max_out entries
// Each entry: Pre_prepare + Prepare certificates
```

### 3.2 Commit Log (`clog`)

```c
Log<Certificate<Commit>> clog;  // Size: max_out entries
// Each entry: Commit certificate (2f+1 slots)
```

### 3.3 Checkpoint Log (`elog`)

```c
Log<Certificate<Checkpoint>> elog;  // Size: max_out/checkpoint_interval entries
// Each entry: Checkpoint certificate (2f+1 slots)
```

---

## 4. ESP32-C3 Memory Budget

Target: ESP32-C3 with 384KB SRAM

| Component | Size (KB) | Notes |
|-----------|-----------|-------|
| Message buffers (send+recv) | 16 | 2 × 8KB |
| Fragment pool (~70 frags) | 50 | ~750B per fragment |
| ESP-NOW internal buffers | 20 | Driver managed |
| Replica logs (window=128) | 80 | Reduced from 256 |
| Static buffers (requests, replies) | 70 | Pre-allocated |
| Stack + application code | 60 | FreeRTOS tasks |
| mbedTLS working memory | 10 | Crypto operations |
| **Reserved** | **78** | Safety margin |
| **Total** | **384** | |

### 4.1 Scaling Constraints

| Config | Feasible on ESP32-C3? | Notes |
|--------|----------------------|-------|
| f=1, n=4, window=32 | Yes | Minimal |
| f=2, n=7, window=128 | Yes | Recommended |
| f=2, n=7, window=256 | Tight | May need to reduce msg size |
| f=3, n=10, window=256 | No | Too much memory |

---

## 5. Memory Regions (Static Mode)

### 5.1 Agreement Region

Stores consensus messages (Pre-prepare, Prepare, Commit):

```c
struct agreement_region {
    // Pre-prepare messages: WINDOW_SIZE slots
    Pre_prepare_rep pre_prepares[WINDOW_SIZE];

    // Prepare messages: WINDOW_SIZE * (n-1) slots
    Prepare_rep prepares[WINDOW_SIZE * (MAX_NUM_REPLICAS - 1)];

    // Commit messages: WINDOW_SIZE * (n-1) slots
    Commit_rep commits[WINDOW_SIZE * (MAX_NUM_REPLICAS - 1)];
};
```

### 5.2 Checkpoint Region

Stores checkpoint messages and state:

```c
struct checkpoint_region {
    // Checkpoint messages: (WINDOW_SIZE/INTERVAL) * n slots
    Checkpoint_rep checkpoints[WINDOW_SIZE / CHECKPOINT_INTERVAL * MAX_NUM_REPLICAS];

    // Above-window checkpoints: n slots
    Checkpoint_rep above_window[MAX_NUM_REPLICAS];
};
```

### 5.3 Special Region

Persists across reboots (keys, replies):

```c
struct special_region {
    // Request storage: n_clients slots
    Request_rep requests[MAX_NUM_CLIENTS];

    // Reply storage: n_clients slots
    Reply_rep replies[MAX_NUM_CLIENTS];

    // View change messages: n slots
    View_change_rep view_changes[MAX_NUM_REPLICAS];
};
```

### 5.4 Scratch Allocator

Temporary buffers for message construction:

```c
struct scratch_allocator {
    // Temporary message buffers
    char buffer[MAX_MESSAGE_SIZE * 4];  // 4 concurrent messages
};
```

---

## 6. Message Memory Layout

### 6.1 Allocation

```c
// Dynamic mode
void* msg = log_allocator.malloc(ALIGNED_SIZE(size));

// Static mode
void* msg = scratch_allocator.alloc(ALIGNED_SIZE(size));
// or for persistent:
void* msg = agreement_region.store(msg);
```

### 6.2 Alignment

All messages must be 8-byte aligned:

```c
#define ALIGNMENT 8
#define ALIGNED(x) (((x) % ALIGNMENT) == 0)
#define ALIGNED_SIZE(x) (((x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
```

### 6.3 Message Lifecycle

```
1. Allocate (scratch region)
2. Fill header + payload
3. Authenticate (add MAC/signature)
4. Send via transport
5. Free (scratch region)
   OR
5. Persist (agreement/checkpoint region) - if stored in certificate
```

---

## 7. Per-Principal Memory

Each replica stores per-peer state:

```c
class Principal {
    int id;
    Addr addr;                    // sockaddr_in (16 bytes)
    PublicKey* pkey;              // RSA public key (~300 bytes)
    unsigned char kin[32];       // Incoming session key
    unsigned char kout[32];      // Outgoing session key
    unsigned long tstamp;        // Last key rotation timestamp
    Hmac hmac_in;                // HMAC context (~100 bytes)
    Hmac hmac_out;               // HMAC context (~100 bytes)
    size_t ssize;                // Signature size
};
// Total per principal: ~600 bytes
// For n=7: ~4.2 KB
```

---

## 8. Reply Information

Client-side reply deduplication:

```c
class Rep_info {
    // Per-client reply storage
    Reply* replies[MAX_NUM_CLIENTS];  // Cached replies
    Request_id last_rid[MAX_NUM_CLIENTS];  // Last request IDs
};
// Total: ~n_clients * (MAX_MESSAGE_SIZE + 8) bytes
```

---

## 9. Timer Memory

```c
// View change timer
ITimer* vtimer;  // ~64 bytes

// Authentication timer  
ITimer* atimer;  // ~64 bytes

// Status timer
ITimer* stimer;  // ~64 bytes

// Recovery timer
ITimer* rtimer;  // ~64 bytes

// Null request timer
ITimer* ntimer;  // ~64 bytes
// Total timers: ~320 bytes
```

---

## 10. Memory Optimization Tips

### 10.1 Reduce WINDOW_SIZE

Most impactful for memory. Each entry in plog/clog stores a full message.

**Tradeoff:** Smaller window = fewer concurrent requests = lower throughput.

### 10.2 Reduce MAX_MESSAGE_SIZE

Smaller messages = smaller buffers.

**Tradeoff:** More fragmentation on ESP-NOW, smaller request payloads.

### 10.3 Reduce MAX_NUM_REPLICAS

Smaller authenticators, fewer certificate slots.

**Tradeoff:** Lower fault tolerance (f = (n-1)/3).

### 10.4 Use TINY_BFT Mode

```cmake
cmake .. -DTINY_BFT=1
```

Enables static allocation and memory optimizations.

---

## 11. Memory Checklist

- [ ] `WINDOW_SIZE > CHECKPOINT_INTERVAL`
- [ ] `max_request_size <= MAX_MESSAGE_SIZE`
- [ ] `max_view_change_size <= MAX_MESSAGE_SIZE`
- [ ] Total memory fits target (384KB for ESP32-C3)
- [ ] All messages 8-byte aligned
- [ ] No memory leaks in message lifecycle
- [ ] Static allocation for embedded targets
- [ ] Certificate logs sized correctly
- [ ] Fragment pool sized for worst case

---

## Source Files

- `src/parameters.h` - Compile-time constants
- `src/Message.h` - Message allocation
- `src/Log_allocator.h` - Dynamic memory allocator
- `src/scratch_allocator.h` - Scratch memory region
- `src/agreement_region.h` - Agreement region
- `src/checkpoint_region.h` - Checkpoint region
- `src/special_region.h` - Special region (persistent)
- `src/Principal.h` - Per-principal memory
- `src/Rep_info.h` - Reply information storage
- `src/Log.h` - Certificate log template
