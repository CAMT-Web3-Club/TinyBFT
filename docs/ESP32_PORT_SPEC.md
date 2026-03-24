# TinyBFT ESP-NOW Port Specification

## Document Information

| Field | Value |
|-------|-------|
| **Project** | TinyBFT ESP-NOW Port |
| **Target MCU** | ESP32-C3 (RISC-V) |
| **ESP-IDF Version** | 5.5.3 |
| **Protocol** | ESP-NOW (v2.0) |
| **Replicas** | 7 (n=7, f=2, quorum=5) |
| **Crypto** | mbedTLS 3.x (built into ESP-IDF) |
| **Debug** | JTAG |

---

# Part 1: Project History

## What Was Done Previously

### Fixed Issues

1. **RSA Key Dangling Pointer Bug** - Critical bug where `mbedtls_pk_context` was a local variable in RsaPrivateKey/RsaPublicKey constructors, causing use-after-free after constructor completed

2. **Wrong Key Parser** - RsaPublicKey used `mbedtls_pk_parse_public_keyfile()` but test keys were private keys, needed `mbedtls_pk_parse_keyfile()`

3. **MAX_MESSAGE_SIZE Too Small** - View change messages required 13472 bytes but MAX_MESSAGE_SIZE was only 9000, increased to 16384

4. **Node ID Detection** - Original code relied on hostname matching which failed, added port-based fallback for replicas

5. **Key Format Issues** - mbedTLS requires RSA private keys in PKCS#1 format (BEGIN RSA PRIVATE KEY), not PKCS#8 or OpenSSH format

6. **Public Headers Organization** - Moved types.h, State_defs.h, Modify.h to include/ to fix multiple definition errors

### Accomplished

- Fixed RsaPrivateKey dangling pointer bug (added pk_ctx_ member variable)
- Fixed RsaPublicKey dangling pointer bug + key parser issue
- Fixed MAX_MESSAGE_SIZE (9000 → 16384)
- Fixed Node.cc port-based node_id detection
- Created working examples (simple_client.cc, simple_replica.cc)
- Updated README.md with build instructions, key generation, config format
- Updated ARCHITECTURE.md
- Generated valid RSA keys using ssh-keygen + PEM conversion
- Successfully tested 4 replicas + 1 client initialization on Linux
- **Full consensus test working** - SET/GET commands with 4 replicas
- Created Transport abstraction layer (UDP + ESP-NOW)
- Created Fragmentation layer for ESP-NOW MTU
- Created ESP-IDF component structure

---

# Part 2: ESP-NOW Port Specification

## 1. Overview

This document specifies the port of TinyBFT from UDP/IP sockets to ESP-NOW protocol for ESP32-C3 microcontrollers. The port maintains backward compatibility with Linux for testing while adding native ESP32-C3 support.

### 1.1 Architecture Goals

1. **Memory Efficiency**: Fit within 384KB SRAM on ESP32-C3
2. **Low Latency**: Leverage ESP-NOW's sub-5ms latency
3. **Backward Compatibility**: Keep UDP transport for Linux testing
4. **Standard Components**: Use ESP-IDF's built-in mbedTLS

---

## 2. Configuration

### 2.1 Build Parameters

| Parameter | Linux Default | ESP32-C3 Value | Notes |
|-----------|---------------|----------------|-------|
| MAX_MESSAGE_SIZE | 16384 | 8192 | Reduced for memory |
| MAX_NUM_REPLICAS | 32 | 7 | Fixed for deployment |
| BLOCK_SIZE | 4096 | 4096 | Unchanged |
| WINDOW_SIZE | 256 | 128 | Reduced for memory |
| CHECKPOINT_INTERVAL | 128 | 128 | Unchanged |
| MAX_NUM_CLIENTS | 1 | 1 | Unchanged |

### 2.2 Memory Budget (ESP32-C3)

| Component | Allocation | Notes |
|-----------|------------|-------|
| Message buffers | 16 KB × 2 | Send + receive |
| Fragment pool | ~50 KB | ~70 fragments @ 750B |
| ESP-NOW buffers | 20 KB | Internal |
| Replica state (logs) | 80 KB | Reduced window |
| Static buffers (requests, replies) | ~70 KB | With 8KB max_message |
| Stack + app code | 60 KB | |
| **Reserved** | ~104 KB | Safety margin |
| **Total** | ~384 KB | Exactly fits ESP32-C3 |

---

## 3. Network Transport Layer

### 3.1 Transport Interface

```cpp
// src/Transport.h
#ifndef _Transport_h
#define _Transport_h

#include <cstdint>
#include <cstddef>
#include <vector>
#include <queue>
#include <string>

namespace libbyzea {

class Message;

class Transport {
public:
    virtual ~Transport() = default;
    
    // Initialize transport (socket or ESP-NOW)
    virtual void init() = 0;
    
    // Send message to destination
    virtual void send(Message* m, int dest_id) = 0;
    
    // Receive message (blocking or polling)
    virtual Message* recv() = 0;
    
    // Add peer for communication
    virtual void add_peer(int id, const uint8_t* mac_or_ip) = 0;
    
    // Check if transport is ready
    virtual bool is_ready() = 0;
    
    // Get maximum payload size
    virtual size_t max_payload_size() const = 0;
    
    // For debugging: get peer address string
    virtual std::string peer_addr_string(int id) const = 0;
};

} // namespace libbyzea

#endif // _Transport_h
```

### 3.2 UDP Transport (Linux)

```cpp
// src/UdpTransport.h
class UdpTransport : public Transport {
    int sock_;
    std::vector<Addr> peer_addrs_;
    
public:
    void init() override;
    void send(Message* m, int dest_id) override;
    Message* recv() override;
    void add_peer(int id, const uint8_t* ip) override;
    bool is_ready() override;
    size_t max_payload_size() const override { return 65507; } // UDP max
    std::string peer_addr_string(int id) const override;
};
```

### 3.3 ESP-NOW Transport (ESP32-C3)

```cpp
// src/EspNowTransport.h
class EspNowTransport : public Transport {
    static constexpr size_t kMaxFragment = 1470;  // ESP-NOW v2.0
    static constexpr uint8_t kMaxPeers = 20;
    
    std::vector<esp_now_peer_info_t> peers_;
    std::queue<Message*> recv_queue_;
    bool initialized_ = false;
    
public:
    void init() override;
    void send(Message* m, int dest_id) override;
    Message* recv() override;
    void add_peer(int id, const uint8_t* mac) override;
    bool is_ready() override { return initialized_; }
    size_t max_payload_size() const override { return kMaxFragment; }
    std::string peer_addr_string(int id) const override;
};
```

### 3.4 Transport Factory

```cpp
// src/TransportFactory.h
enum class TransportType {
    UDP,      // Linux testing
    ESP_NOW   // ESP32-C3 deployment
};

class TransportFactory {
public:
    static Transport* create(TransportType type);
};
```

---

## 4. Fragmentation Layer

### 4.1 Why Fragmentation is Needed

ESP-NOW v2.0 has a maximum payload of **1470 bytes**. TinyBFT messages can be up to **8KB** (reduced from 16KB for ESP32-C3), so messages must be fragmented.

### 4.2 Fragment Header Format

```
+----------------+----------------+----------------+
|  FragHeader    |   Payload      |   Padding      |
|   (16 bytes)   |  (≤1454 bytes) |   (optional)   |
+----------------+----------------+----------------+
```

```cpp
// src/Fragmentation.h
#pragma pack(push, 1)
struct FragHeader {
    uint32_t msg_id;      // Unique message identifier
    uint16_t seq;         // Fragment sequence (0, 1, 2, ...)
    uint16_t total;       // Total fragments
    uint16_t size;        // This fragment's data size
    uint16_t dest_id;     // Destination replica ID
    uint8_t  protocol;    // Protocol version
    uint8_t  flags;       // Reserved for flags
}; // 16 bytes

struct FragMessage {
    FragHeader header;
    uint8_t data[];
};
#pragma pack(pop)

constexpr size_t kFragHeaderSize = sizeof(FragHeader);
constexpr size_t kMaxPayload = 1470 - kFragHeaderSize;  // ~1454 bytes
```

### 4.3 Fragment Sizes

| Original Message | Fragments (1454B each) |
|-----------------|------------------------|
| 1 KB | 1 |
| 4 KB | 3 |
| 8 KB | 6 |

### 4.4 Fragmentation API

```cpp
// src/Fragmentation.h
class Fragmentation {
public:
    // Fragment a large message into smaller chunks
    // Returns vector of fragment messages (caller must delete)
    static std::vector<Message*> fragment(Message* msg, int dest_id);
    
    // Reassemble fragments back into original message
    // Returns reassembled message (caller must delete) or nullptr if incomplete
    static Message* reassemble(Message* fragment);
    
    // Check if a message is a fragment
    static bool is_fragment(const Message* msg);
    
    // Get fragment info
    static FragHeader get_header(const Message* msg);
    
    // Clear pending fragments (timeout handler)
    static void clear_pending();
};
```

### 4.5 Fragment Buffer Pool

```cpp
// Pre-allocated static buffers to avoid heap fragmentation
static constexpr size_t kNumFragBuffers = 70;
static uint8_t frag_buffers[kNumFragBuffers][1470] __attribute__((aligned(4)));
```

### 4.6 Fragment States

```
WAITING ──(receive all)──> COMPLETE
    │
    └──(timeout 5s)─────> DROPPED
```

---

## 5. Configuration File Format

### 5.1 Linux/UDP Config (Existing)

```
service_name
f
auth_timeout
num_principals
multicast_addr multicast_port
hostname0 ip0 port0 keyfile0
hostname1 ip1 port1 keyfile1
...
view_change_timeout
status_timeout
recovery_timeout
```

### 5.2 ESP-NOW Config (New)

```
service_name
f
auth_timeout
num_principals
broadcast_mac broadcast_channel  # For ESP-NOW, channel is more important
replica0 mac0 port0 keyfile0
replica1 mac1 port1 keyfile1
...
view_change_timeout
status_timeout
recovery_timeout
```

### 5.3 Example: 7 Replicas Config

```
test_bft
2                    # f=2 (max faulty)
1800000             # auth timeout (ms)
8                   # num_principals (7 replicas + 1 client)
FF:FF:FF:FF:FF:FF 6 # broadcast MAC, channel 6
replica0 AA:BB:CC:DD:EE:F0 9001 r0.pub
replica1 AA:BB:CC:DD:EE:F1 9001 r1.pub
replica2 AA:BB:CC:DD:EE:F2 9001 r2.pub
replica3 AA:BB:CC:DD:EE:F3 9001 r3.pub
replica4 AA:BB:CC:DD:EE:F4 9001 r4.pub
replica5 AA:BB:CC:DD:EE:F5 9001 r5.pub
replica6 AA:BB:CC:DD:EE:F6 9001 r6.pub
client0  AA:BB:CC:DD:EE:F7 9002 c0.pub
5000                  # view change timeout (ms)
150                  # status timeout (ms)
60000                # recovery timeout (ms)
```

---

## 6. ESP-IDF Component Structure

### 6.1 Directory Structure

```
components/tinybft/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── tinybft.h
├── idf_component.yml
└── src/
    ├── Client.cc
    ├── Replica.cc
    ├── Node.cc           # Modified: uses Transport*
    ├── Transport.h       # New
    ├── Transport.cc      # New
    ├── EspNowTransport.cc  # New
    ├── UdpTransport.cc    # Modified
    ├── Fragmentation.cc  # New
    ├── Fragmentation.h   # New
    ├── Principal.cc      # Modified: MAC address
    ├── Message.cc
    ├── Request.cc
    ├── Reply.cc
    ├── Digest.cc
    ├── dsum.cc
    ├── Commit.cc
    ├── Pre_prepare.cc
    ├── Prepare.cc
    ├── Checkpoint.cc
    ├── View_change.cc
    ├── New_view.cc
    ├── Status.cc
    ├── Fetch.cc
    ├── ITimer.cc
    ├── Principal.cc
    ├── Log_allocator.cc
    ├── State.cc
    ├── libbyz.cc
    ├── rsa_private_key.cc
    ├── rsa_public_key.cc
    ├── hmac.cc
    ├── platform.cc
    └── (other TinyBFT sources)
```

### 6.2 CMakeLists.txt

```cmake
idf_component_register(
    SRCS 
        "src/Client.cc"
        "src/Replica.cc"
        "src/Node.cc"
        "src/Transport.cc"
        "src/EspNowTransport.cc"
        "src/Fragmentation.cc"
        # ... other sources
    INCLUDE_DIRS 
        "include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    REQUIRES 
        esp_wifi
        esp_now
        mbedtls
        nvs_flash
    PRIV_REQUIRES 
        esp_timer
        heap
)

# Apply TinyBFT-specific compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
)
```

### 6.3 Kconfig

```makefile
config TINYBFT_ESPNOW
    bool "Use ESP-NOW transport"
    default y
    help
        Enable ESP-NOW transport instead of UDP.

config TINYBFT_MAX_MESSAGE_SIZE
    int "Maximum message size"
    default 8192
    range 1024 16384
    help
        Maximum size of messages in bytes.

config TINYBFT_NUM_REPLICAS
    int "Number of replicas"
    default 7
    range 4 19
    help
        Number of replicas in the BFT cluster.

config TINYBFT_FRAG_TIMEOUT_MS
    int "Fragment reassembly timeout"
    default 5000
    range 1000 30000
    help
        Timeout in ms for fragment reassembly.
```

---

## 7. TinyBFT Client Allocation Optimization

### How It Works

When `TINY_BFT=1` is enabled (which sets `STATIC_LOG_ALLOCATOR`), TinyBFT uses **pre-allocated static buffers** instead of heap allocation:

```cpp
// From special_region.cc
static RequestBlock requests[max_num_clients + Max_num_replicas];
static ReplyBlock replies[F + 1];
```

### Client vs Replica Behavior

**For Replicas** (`Byz_alloc_request`):
```cpp
if (libbyzea::node->is_replica(libbyzea::node->id())) {
    request = libbyzea::special_region::new_request(...);  // Uses static buffer
}
```

**For Clients**:
```cpp
else {
    request = new libbyzea::Request(...);  // Still uses HEAP allocation
}
```

### Memory Impact

| Component | Size | Notes |
|-----------|------|-------|
| `requests[]` | (max_num_clients + MAX_NUM_REPLICAS) × max_request_size | ~17 × 8KB = 136KB |
| `replies[]` | (F + 1) × Max_rep_size | ~3 × 2KB = 6KB |
| Other static | New_view, View_change, etc. | Varies |

**Total static allocation**: ~150-200KB (depending on MAX_MESSAGE_SIZE)

### ESP32-C3 Impact

The static allocation optimization is already in place and works well for ESP32-C3. With MAX_MESSAGE_SIZE=8192, memory usage is reduced by 50% compared to 16KB.

---

## 8. Error Handling

### 8.1 ESP-NOW Specific Errors

| Error Code | Description | Recovery |
|------------|-------------|----------|
| ESP_NOW_NO_MEM | Out of memory | Reduce buffer sizes |
| ESP_NOW_NOT_FOUND | Peer not found | Re-add peer |
| ESP_NOW_FAIL | Send failed | Retry with backoff |
| ESP_NOW_SEND_FAIL | Transmission failed | Retransmit fragment |

### 8.2 Fragment Timeout

```cpp
void handle_frag_timeout(uint32_t msg_id) {
    // Remove incomplete fragment assembly
    Fragmentation::clear_pending(msg_id);
    // Request retransmission from sender
    // (send NACK to origin)
}
```

---

## 9. Implementation Checklist

### Phase 1: Transport Abstraction
- [x] Create Transport interface (Transport.h)
- [x] Modify Node.cc to use Transport*
- [x] Implement UdpTransport for Linux

### Phase 2: ESP-NOW Implementation
- [x] Create EspNowTransport class
- [x] Implement WiFi + ESP-NOW initialization
- [x] Implement send/receive callbacks (ESP-IDF 5.5 API)
- [x] Add peer management

### Phase 3: Fragmentation
- [x] Implement Fragmentation class
- [x] Add fragment header format
- [x] Implement reassembly logic
- [x] Add timeout handling

### Phase 4: ESP-IDF Integration
- [x] Create component structure (components/main/)
- [x] Write CMakeLists.txt
- [ ] Write Kconfig (optional)
- [x] Test build ✅

### Phase 5: Configuration
- [x] Update config file parser (SPIFFS-based)
- [x] Add MAC address support
- [ ] Test with 7-node config

### Phase 6: Testing
- [ ] 2-node basic test
- [ ] 7-node consensus test
- [ ] Fault tolerance test

---

## 10. Compatibility Matrix

| Feature | Linux | ESP32-C3 |
|---------|-------|----------|
| UDP Transport | ✅ | ✅ (via Wi-Fi) |
| ESP-NOW Transport | ❌ | ✅ |
| mbedTLS | ✅ | ✅ |
| RSA Keys | ✅ | ✅ |
| 7 Replicas | ✅ | ✅ |
| Fragmentation | ✅ (not needed) | ✅ |

---

# Part 3: Deployment Plan

## Hardware Setup

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32-C3 | 7 units | Dev board (e.g., ESP32-C3-DevKitM-1) |
| USB-C cable | 7 | Power + programming |
| Computer | 1 | For flashing + JTAG debugging |
| USB hub | 1 | Connect all ESP32-C3s |

## MAC Address Collection

Each ESP32-C3 needs a unique MAC address:

1. **Flash a simple sketch** to each ESP32-C3 that prints its MAC address
2. **Record the MACs** (format: `AA:BB:CC:DD:EE:FF`)
3. **Create config file** with all 7 MAC addresses

## Deployment Steps

```
Step 1: Collect MAC Addresses
├── Flash "get_mac.ino" to each ESP32-C3
├── Open serial monitor (115200 baud)
├── Record output: "STA MAC: AA:BB:CC:DD:EE:F0"
└── Repeat for 7 devices

Step 2: Generate RSA Keys
├── Generate 7 replica keys (r0-r6.pem)
├── Generate 1 client key (client0.pem)
└── Convert to PKCS#1 format if needed

Step 3: Create Configuration
├── Edit test_espnow.conf with MAC addresses
├── Ensure all use same WiFi channel
└── Set timeouts appropriate for ESP-NOW

Step 4: Build ESP-IDF Project
├── idf.py set-target esp32c3
├── idf.py menuconfig (enable ESP-NOW, set options)
├── idf.py build
└── ...

Step 5: Flash All Devices
├── Connect JTAG debugger (or use USB serial)
├── idf.py -p /dev/ttyUSB0 flash (for each device)
├── Each device: specify node_id (0-6)
└── Monitor output: idf.py -p /dev/ttyUSB0 monitor

Step 6: Run Consensus Test
├── Start all 7 replicas
├── Start client
├── Send test requests
├── Verify consensus
└── Kill 2 replicas, verify fault tolerance
```

## Device Addressing

| Device | Role | MAC (example) | Port |
|--------|------|---------------|------|
| ESP32-C3 #1 | Replica 0 | AA:BB:CC:DD:EE:F0 | 9001 |
| ESP32-C3 #2 | Replica 1 | AA:BB:CC:DD:EE:F1 | 9001 |
| ESP32-C3 #3 | Replica 2 | AA:BB:CC:DD:EE:F2 | 9001 |
| ESP32-C3 #4 | Replica 3 | AA:BB:CC:DD:EE:F3 | 9001 |
| ESP32-C3 #5 | Replica 4 | AA:BB:CC:DD:EE:F4 | 9001 |
| ESP32-C3 #6 | Replica 5 | AA:BB:CC:DD:EE:F5 | 9001 |
| ESP32-C3 #7 | Replica 6 | AA:BB:CC:DD:EE:F6 | 9001 |
| Laptop/ESP32 | Client | - | 9002 |

## Testing Checklist

| Test | Description | Success Criteria |
|------|-------------|------------------|
| Basic | 7 replicas start | All show "Replica ready" |
| Consensus | Client sends SET/GET | Correct response received |
| Fault-1 | Kill 1 replica | System continues (5/7) |
| Fault-2 | Kill 2 replicas | System continues (4/7) |
| Fault-3 | Kill 3 replicas | System fails (3/7 = Byzantine) |
| View Change | Kill primary | New primary elected |
| Stress | 100 rapid requests | All processed correctly |

## Performance Benchmarks

| Metric | Target | Method |
|--------|--------|--------|
| Latency (1 msg) | < 5ms | Round-trip time |
| Throughput | > 100 req/s | Requests per second |
| Memory usage | < 250KB | heap_caps_get_free_size |
| CPU usage | < 80% | esp_cpu_get_cycle_count |

---

# Part 4: Crypto: mbedTLS vs Alternatives

## Decision: Use mbedTLS

**Yes, use mbedTLS** - here's why:

| Crypto Option | Pros | Cons |
|--------------|------|------|
| **mbedTLS (current)** | Already implemented in TinyBFT, works on ESP-IDF | Software fallback if HW not used |
| **wolfSSL** | Faster HW acceleration on RISC-V | New integration required |
| **ESP32 HW crypto (legacy)** | Very fast | Deprecated in ESP-IDF v6+ |

### ESP-IDF Includes mbedTLS with Hardware Acceleration

ESP-IDF includes mbedTLS with hardware acceleration support on ESP32-C3:

- `CONFIG_MBEDTLS_HARDWARE_SHA` - Hardware SHA
- `CONFIG_MBEDTLS_HARDWARE_AES` - Hardware AES  
- `CONFIG_MBEDTLS_HARDWARE_MPI` - Hardware RSA (big number math)

**Your existing TinyBFT code will work** - just need to ensure ESP-IDF mbedTLS is enabled. No code changes needed for RSA signing/verification.

---

# References

- [ESP-NOW API Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/network/esp_now.html)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/)
- [mbedTLS in ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mbedtls.html)
- [TinyBFT Repository](https://github.com/anomalyco/TinyBFT)

---

**Document Version**: 1.1  
**Last Updated**: 2026-03-24  
**Author**: TinyBFT ESP-NOW Port Team
