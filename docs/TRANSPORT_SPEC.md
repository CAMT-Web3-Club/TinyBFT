# TinyBFT Transport Specification

Network transport layer specification for porting to any platform.

## 1. Transport Interface

All transports must implement this abstract interface:

```c
class Transport {
    void init();
    void send(Message* m, int dest_id);
    Message* recv();
    void add_peer(int id, const uint8_t* addr);
    bool is_ready();
    size_t max_payload_size() const;
    string peer_addr_string(int id) const;
    TransportType type() const;
};
```

### Transport Types

```c
enum class TransportType {
    UDP = 1,
    LOOPBACK = 2,
    ESP_NOW = 3
};
```

---

## 2. UDP Transport (Linux)

### 2.1 Socket Configuration

```c
// Create UDP socket
int sock = socket(AF_INET, SOCK_DGRAM, 0);

// Non-blocking
fcntl(sock, F_SETFL, O_NONBLOCK);

// Address reuse
int opt = 1;
setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### 2.2 Multicast Support

**Group address:** Read from config file (e.g., `239.255.0.1`)
**Port:** Read from config file (e.g., `5678`)

```c
// Join multicast group
struct ip_mreq mreq;
inet_pton(AF_INET, "239.255.0.1", &mreq.imr_multiaddr);
mreq.imr_interface.s_addr = INADDR_ANY;
setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

// Send to multicast group
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(5678);
inet_pton(AF_INET, "239.255.0.1", &addr.sin_addr);
sendto(sock, msg, len, 0, (struct sockaddr*)&addr, sizeof(addr));
```

### 2.3 Unicast Fallback

When `DISABLE_MULTICAST=1` or `NO_IP_MULTICAST` defined:

```c
void send(Message* m, int dest_id) {
    if (dest_id == All_replicas) {
        for (int i = 0; i < num_replicas; i++) {
            send(m, i);  // Unicast to each
        }
        return;
    }
    // Send to specific peer
    sendto(sock, m->contents(), m->size(), 0,
           (struct sockaddr*)&peers[dest_id], sizeof(sockaddr_in));
}
```

### 2.4 Peer Storage

```c
// Fixed array of peer addresses
sockaddr_in peers[MAX_NUM_REPLICAS];  // Indexed by replica ID

// Populated from config file:
// hostname ip_address port keyfile_path
peers[0] = {sin_family=AF_INET, sin_port=5679, sin_addr=127.0.0.1}
```

### 2.5 Max Payload

```c
size_t max_payload_size() const {
    return 65507;  // UDP maximum
}
```

---

## 3. Loopback Transport (Testing)

### 3.1 In-Memory Queue

For single-machine testing without network:

```c
class LoopbackTransport {
    mutex queue_mutex;
    queue<Message*> msg_queue;

    void send(Message* m, int dest_id) {
        lock_guard<mutex> lock(queue_mutex);
        msg_queue.push(m->clone());
    }

    Message* recv() {
        lock_guard<mutex> lock(queue_mutex);
        if (msg_queue.empty()) return nullptr;
        Message* m = msg_queue.front();
        msg_queue.pop();
        return m;
    }
};
```

### 3.2 Configuration

- No network setup required
- All replicas run in same process
- Useful for unit testing protocol correctness

---

## 4. ESP-NOW Transport (ESP32)

### 4.1 Constants

```c
static constexpr size_t kMaxFragment = 1470;  // ESP-NOW v2.0 MTU
static constexpr uint8_t kMaxPeers = 20;       // Maximum peer count
static constexpr int kDefaultChannel = 6;      // WiFi channel
```

### 4.2 Initialization Sequence

```c
void EspNowTransport::init() {
    // 1. Initialize WiFi driver
    esp_wifi_init(&wifi_cfg);

    // 2. Set station mode
    esp_wifi_set_mode(WIFI_MODE_STA);

    // 3. Set WiFi channel
    esp_wifi_set_channel(wifi_channel_, WIFI_SECOND_CHAN_NONE);

    // 4. Start WiFi
    esp_wifi_start();

    // 5. Initialize ESP-NOW
    esp_now_init();

    // 6. Register callbacks
    esp_now_register_send_cb(on_send_complete);
    esp_now_register_recv_cb(on_receive);

    // 7. Set ready
    ready_ = true;
}
```

### 4.3 Peer Management

```c
void EspNowTransport::add_peer(int id, const uint8_t* mac) {
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = wifi_channel_;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;  // No ESP-NOW encryption (app-layer auth)

    esp_now_add_peer(&peer);

    // Store for lookup
    peers_[id] = {mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]};
    mac_to_id_[mac_to_key(mac)] = id;
}
```

### 4.4 Send Flow

```c
void EspNowTransport::send(Message* m, int dest_id) {
    if (dest_id == All_replicas) {
        // ESP-NOW has no true multicast - send to each peer
        for (int i = 0; i < num_replicas; i++) {
            if (i != my_id) send(m, i);
        }
        return;
    }

    // Fragment if necessary
    if (m->size() <= kMaxFragment) {
        esp_now_send(peers_[dest_id].data(), (uint8_t*)m->contents(), m->size());
    } else {
        auto fragments = Fragmentation::fragment(m, dest_id);
        for (auto* frag : fragments) {
            esp_now_send(peers_[dest_id].data(),
                        (uint8_t*)frag->contents(), frag->size());
            delete frag;
        }
    }
}
```

### 4.5 Receive Callback

```c
static void on_receive(const uint8_t* mac, const uint8_t* data, int len) {
    // Look up sender ID from MAC
    int sender_id = mac_to_id_[mac_to_key(mac)];

    // Check if fragment
    if (Fragmentation::is_fragment(data, len)) {
        Message* complete = Fragmentation::reassemble(data, len, sender_id);
        if (complete) {
            recv_queue.push(complete);
        }
    } else {
        recv_queue.push(new Message(data, len));
    }
}
```

### 4.6 Max Payload

```c
size_t max_payload_size() const {
    return kMaxFragment - sizeof(FragHeader);  // 1470 - 16 = 1454
}
```

---

## 5. Peer Discovery

### 5.1 Linux (Config File)

Peers specified in config file:
```
# Line format: hostname ip_address port keyfile_path
localhost 127.0.0.1 5679 /path/to/r0.pem
localhost 127.0.0.1 5680 /path/to/r1.pem
...
```

Parsed by `Node` constructor, populates `peers[]` array.

### 5.2 ESP32 (SPIFFS Config)

Peers loaded from SPIFFS filesystem:

**Config file format (`/spiffs/config`):**
```
# Comments ignored
# Format: id MAC_address
0 AA:BB:CC:DD:EE:F0
1 AA:BB:CC:DD:EE:F1
2 AA:BB:CC:DD:EE:F2
```

**Loading code:**
```c
void Node::load_peers_from_spiffs() {
    FILE* f = fopen("/spiffs/config", "r");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') continue;
        int id;
        uint8_t mac[6];
        sscanf(line, "%d %hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &id, &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        if (id != node_id_) {
            transport->add_peer(id, mac);
        }
    }
    fclose(f);
}
```

---

## 6. Platform Detection

### 6.1 Compile-Time Selection

```c
#ifdef ESP_PLATFORM
    // ESP32 code
    #include "EspNowTransport.h"
#else
    // Linux code
    #include "UdpTransport.h"
#endif
```

### 6.2 Factory Method

```c
std::unique_ptr<Transport> Transport::create(TransportType type) {
    switch (type) {
        case TransportType::UDP:
            return std::make_unique<UdpTransport>();
        case TransportType::LOOPBACK:
            return std::make_unique<LoopbackTransport>();
        case TransportType::ESP_NOW:
#ifdef ESP_PLATFORM
            return std::make_unique<EspNowTransport>();
#else
            fprintf(stderr, "ESP_NOW not available on this platform\n");
            return nullptr;
#endif
    }
}
```

---

## 7. ESP-NOW Integration Details

### 7.1 WiFi Configuration

```c
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
esp_wifi_start();
```

### 7.2 ESP-NOW Callbacks

```c
// Send completion callback
void on_send_complete(const uint8_t* mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        // Handle send failure (retry or log)
    }
}

// Receive callback (called from WiFi task)
void on_receive(const esp_now_recv_info_t* info,
                const uint8_t* data, int len) {
    // Queue message for processing
}
```

### 7.3 Power Management

ESP-NOW works with WiFi power save disabled:
```c
esp_wifi_set_ps(WIFI_PS_NONE);  // No power save for low latency
```

---

## 8. Message Flow Summary

```
Application          Transport              Network
    |                    |                      |
    |--- send(m, 0) --->|                      |
    |                    |-- fragment if large   |
    |                    |-- UDP/ESP-NOW send -->|
    |                    |                      |
    |                    |<-- UDP/ESP-NOW recv --|
    |                    |-- reassemble frags    |
    |<-- recv() ---------|                      |
    |                    |                      |
```

---

## 9. Transport Checklist

- [ ] `init()` initializes hardware/network
- [ ] `send()` handles `All_replicas` (multicast or loop)
- [ ] `recv()` is non-blocking, returns `nullptr` if no message
- [ ] `add_peer()` registers peer address
- [ ] `is_ready()` returns true after initialization
- [ ] `max_payload_size()` returns correct value
- [ ] Fragmentation for messages > MTU
- [ ] Reassembly with timeout handling
- [ ] Peer discovery from config
- [ ] Graceful error handling for network failures

---

## Source Files

- `src/Transport.h` - Transport interface and factory
- `src/Transport.cc` - UDP and Loopback implementations
- `src/EspNowTransport.h` - ESP-NOW transport header
- `src/EspNowTransport.cc` - ESP-NOW implementation
- `src/Fragmentation.h` - Fragment header
- `src/Fragmentation.cc` - Fragmentation/reassembly logic
- `src/Node.cc` - Peer discovery and transport initialization
