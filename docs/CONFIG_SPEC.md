# TinyBFT Configuration Specification

Configuration file format, compile-time constants, and build options.

## 1. Compile-Time Constants

### 1.1 Core Protocol Parameters

| Constant | Default (ESP32) | Range | Description |
|----------|-----------------|-------|-------------|
| `MAX_MESSAGE_SIZE` | 8192 | 1024-16384 | Maximum message size (bytes) |
| `MAX_NUM_REPLICAS` | 7 | 4-32 | Maximum replicas in cluster |
| `WINDOW_SIZE` | 128 | 32-256 | Outstanding requests window |
| `MAX_NUM_CLIENTS` | 1 | 1-10 | Maximum concurrent clients |
| `CHECKPOINT_INTERVAL` | 128 | 16-256 | Checkpoint every N seqnos |
| `BLOCK_SIZE` | 4096 | - | State block size (power of 2) |

### 1.2 Transport Parameters

| Constant | Default | Description |
|----------|---------|-------------|
| `TINYBFT_TRANSPORT` | `ESP_NOW` | Transport layer (UDP or ESP_NOW) |
| `DISABLE_MULTICAST` | 0 | Use unicast instead of multicast |
| `TINYBFT_WIFI_CHANNEL` | 6 | ESP-NOW WiFi channel (1-14) |
| `TINYBFT_FRAG_TIMEOUT_MS` | 5000 | Fragment reassembly timeout (ms) |

### 1.3 Derived Constants

```c
namespace libbyzea {
    constexpr int Max_num_replicas = MAX_NUM_REPLICAS;
    constexpr int checkpoint_interval = CHECKPOINT_INTERVAL;
    constexpr int max_out = WINDOW_SIZE;
    constexpr int max_num_clients = MAX_NUM_CLIENTS;
    constexpr int F = MAX_NUM_REPLICAS / 3;
    constexpr unsigned AUTHENTICATOR_SIZE = 32 * (MAX_NUM_REPLICAS - 1);  // HMAC mode
}
```

### 1.4 Constraint: `max_out > checkpoint_interval`

Required for the algorithm to make progress. With defaults (256 > 128), satisfied.

---

## 2. Configuration File Format

### 2.1 File Structure

The config file is parsed sequentially with `fscanf()`:

```
Line 1:  service_name (string, max 256 chars)
Line 2:  max_faulty (int) - f value
Line 3:  auth_timeout_ms (int)
Line 4:  num_principals (int) - replicas + clients
Line 5:  multicast_addr port (string + short)
Lines 6 to (5 + num_principals):
         hostname ip_address port keyfile_path (per principal)
Lines (6 + num_principals) to (8 + num_principals):
         view_change_timeout_ms (int)
         status_timeout_ms (int)
         recovery_timeout_ms (int)
```

### 2.2 Parsing Logic

```c
// Line 1: Service name
fscanf(config_file, "%256s\n", service_name);

// Line 2: Max faulty (f)
fscanf(config_file, "%d\n", &max_faulty);
num_replicas = 3 * max_faulty + 1;  // n = 3f + 1
threshold = num_replicas - max_faulty;  // quorum = 2f + 1

// Line 3: Auth timeout
int at;
fscanf(config_file, "%d\n", &at);

// Line 4: Number of principals
fscanf(config_file, "%d\n", &num_principals);

// Line 5: Multicast group
char addr_buff[256];
short port;
fscanf(config_file, "%256s %hd\n", addr_buff, &port);

// Lines 6+: Principal definitions
for (int i = 0; i < num_principals; i++) {
    char host_name[64];
    char addr_buff[32];
    short port;
    char public_keyfile[1024];
    fscanf(config_file, "%64s %32s %hd %1023s \n",
           host_name, addr_buff, &port, public_keyfile);
    // Store: peers[i] = {host_name, addr, port, keyfile}
}

// After principals (replica only):
int vt, st, rt;
fscanf(config_file, "%d\n", &vt);  // View change timeout
fscanf(config_file, "%d\n", &st);  // Status timeout
fscanf(config_file, "%d\n", &rt);  // Recovery timeout
```

### 2.3 Example: 4 Replicas (f=1)

```
test_bft
1
1800000
5
239.255.0.1 5678
localhost 127.0.0.1 5679 /home/user/priv/r0.pem
localhost 127.0.0.1 5680 /home/user/priv/r1.pem
localhost 127.0.0.1 5681 /home/user/priv/r2.pem
localhost 127.0.0.1 5682 /home/user/priv/r3.pem
localhost 127.0.0.1 5683 /home/user/priv/client0.pem
10000
1000
60000
```

**Parameters:** n=4, f=1, quorum=3

### 2.4 Example: 7 Replicas (f=2)

```
test_bft
2
1800000
8
239.255.0.1 5679
localhost 127.0.0.1 7001 /home/user/priv/r0.pem
localhost 127.0.0.1 7002 /home/user/priv/r1.pem
localhost 127.0.0.1 7003 /home/user/priv/r2.pem
localhost 127.0.0.1 7004 /home/user/priv/r3.pem
localhost 127.0.0.1 7005 /home/user/priv/r4.pem
localhost 127.0.0.1 7006 /home/user/priv/r5.pem
localhost 127.0.0.1 7007 /home/user/priv/r6.pem
localhost 127.0.0.1 7008 /home/user/priv/client0.pem
5000
150
60000
```

**Parameters:** n=7, f=2, quorum=5

### 2.5 ESP32 Config (SPIFFS)

On ESP32, peer discovery uses a separate file:

**`/spiffs/config`:**
```
# Format: id MAC_address
0 AA:BB:CC:DD:EE:F0
1 AA:BB:CC:DD:EE:F1
2 AA:BB:CC:DD:EE:F2
```

**`/spiffs/tinybft.conf`:**
```
# Replica ID
node_id=0
# WiFi channel
wifi_channel=6
# Key files
priv_key=/spiffs/priv/r0.pem
pub_key=/spiffs/priv/r0.pub
```

---

## 3. ESP-IDF Build Options

### 3.1 Build Commands

```bash
# Build using Docker (recommended)
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py build

# Set target
idf.py set-target esp32c3

# Configure via menuconfig
idf.py menuconfig

# Flash to device
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3.2 Build Configuration Matrix

| Transport | Message Size | Window | Replicas |
|-----------|-------------|--------|----------|
| UDP | 8192 | 128 | 7 |
| ESP-NOW | 8192 | 128 | 7 |

---

## 4. ESP-IDF Build Options

### 4.1 Kconfig Options

```
menu "TinyBFT Configuration"
    config TINYBFT_ENABLED
        bool "Enable TinyBFT"
        default y

    choice TINYBFT_TRANSPORT_CHOICE
        prompt "Transport layer"
        default TINYBFT_TRANSPORT_ESPNOW
        
        config TINYBFT_TRANSPORT_UDP
            bool "UDP"
            
        config TINYBFT_TRANSPORT_ESPNOW
            bool "ESP-NOW"
    endchoice

    config TINYBFT_TRANSPORT
        string
        default "UDP" if TINYBFT_TRANSPORT_UDP
        default "ESP_NOW" if TINYBFT_TRANSPORT_ESPNOW

    config TINYBFT_MAX_MESSAGE_SIZE
        int "Maximum message size"
        default 8192
        range 1024 16384

    config TINYBFT_NUM_REPLICAS
        int "Number of replicas"
        default 7
        range 4 19

    config TINYBFT_MAX_NUM_CLIENTS
        int "Maximum clients"
        default 1
        range 1 10

    config TINYBFT_WINDOW_SIZE
        int "Outstanding requests window"
        default 128
        range 32 256

    config TINYBFT_CHECKPOINT_INTERVAL
        int "Checkpoint interval"
        default 128
        range 16 256

    config TINYBFT_WIFI_CHANNEL
        int "WiFi channel (ESP-NOW)"
        default 6
        range 1 14
        
    config TINYBFT_FRAG_TIMEOUT_MS
        int "Fragment reassembly timeout (ms)"
        default 5000
        range 1000 30000
endmenu
```

### 4.2 sdkconfig.defaults

```ini
CONFIG_TINYBFT_ENABLED=y
CONFIG_TINYBFT_TRANSPORT_ESPNOW=y
CONFIG_TINYBFT_TRANSPORT="ESP_NOW"
CONFIG_TINYBFT_MAX_MESSAGE_SIZE=8192
CONFIG_TINYBFT_NUM_REPLICAS=7
CONFIG_TINYBFT_MAX_NUM_CLIENTS=1
CONFIG_TINYBFT_WINDOW_SIZE=128
CONFIG_TINYBFT_CHECKPOINT_INTERVAL=128
CONFIG_TINYBFT_FRAG_TIMEOUT_MS=5000
CONFIG_TINYBFT_WIFI_CHANNEL=6
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_FREERTOS_HZ=1000
```

### 4.3 Component Dependencies

```cmake
idf_component_register(
    # ... source files ...
    REQUIRES 
        mbedtls
        esp_timer
        esp_wifi
        nvs_flash
        spiffs
        json
)
```

---

## 5. Compile Definitions

| Definition | Set By | Effect |
|------------|--------|--------|
| `ESP_PLATFORM` | ESP-IDF | Enables ESP32-specific code |
| `STATIC_LOG_ALLOCATOR` | Build | Static memory allocation |
| `NO_IP_MULTICAST` | `DISABLE_MULTICAST=1` | Force unicast |
| `TINYBFT_TRANSPORT_ESPNOW` | Transport.h | Enable ESP-NOW |
| `TINYBFT_TRANSPORT_LOOPBACK` | Transport.h | Enable loopback |
| `PKEY` | Build | Use RSA signatures instead of HMAC |
| `PRINT_MEM_STATISTICS` | Build | Enable memory tracking |
| `MAX_MESSAGE_SIZE` | CMake/Kconfig | Override message size |
| `MAX_NUM_REPLICAS` | CMake/Kconfig | Override replica count |
| `WINDOW_SIZE` | CMake/Kconfig | Override window size |

---

## 6. Configuration Validation

### 6.1 Required Constraints

```c
static_assert(max_out > checkpoint_interval,
    "WINDOW_SIZE must be > CHECKPOINT_INTERVAL");

static_assert(max_request_size <= MAX_MESSAGE_SIZE,
    "Request too large for message buffer");

static_assert(max_view_change_size <= MAX_MESSAGE_SIZE,
    "View change too large for message buffer");
```

### 6.2 Recommended Configurations

**Development/Testing:**
```
f=1, n=4, WINDOW_SIZE=32, CHECKPOINT_INTERVAL=16
MAX_MESSAGE_SIZE=4096, TRANSPORT=UDP
```

**Production (ESP32):**
```
f=2, n=7, WINDOW_SIZE=128, CHECKPOINT_INTERVAL=128
MAX_MESSAGE_SIZE=8192, TRANSPORT=ESP_NOW
```

---

## Source Files

- `CMakeLists.txt` - Main build configuration
- `src/parameters.h` - Compile-time constants
- `src/Message.h` - Message size limits
- `src/Node.cc` - Config file parsing
- `src/Replica.cc` - Replica config parsing
- `components/tinybft/CMakeLists.txt` - ESP-IDF component config
- `components/tinybft/Kconfig` - ESP-IDF menuconfig
- `sdkconfig.defaults` - ESP-IDF default config
- `test_runtime/test.conf` - Example 4-replica config
- `test_runtime/test_7r.conf` - Example 7-replica config
