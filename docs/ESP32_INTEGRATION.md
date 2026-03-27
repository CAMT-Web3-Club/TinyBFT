# TinyBFT ESP32 Integration Guide

Complete guide for integrating TinyBFT as an ESP-IDF component.

## 1. Component Structure

```
components/tinybft/
├── CMakeLists.txt          # Component build definition
├── idf_component.yml       # Component metadata
├── Kconfig                 # Menuconfig options
├── include/
│   └── tinybft.h          # Public header
├── src/                    # (References ../../src/*.cc)
└── test/
    ├── CMakeLists.txt      # Unity test build
    ├── test_consensus.cc
    ├── test_safety.cc
    └── test_view_change.cc
```

## 2. Component CMakeLists.txt

```cmake
set(TINYBFT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

idf_component_register(
    SRCS
        "${TINYBFT_SRC_DIR}/Transport.cc"
        "${TINYBFT_SRC_DIR}/EspNowTransport.cc"
        "${TINYBFT_SRC_DIR}/Fragmentation.cc"
        "${TINYBFT_SRC_DIR}/Node.cc"
        "${TINYBFT_SRC_DIR}/Replica.cc"
        "${TINYBFT_SRC_DIR}/Client.cc"
        "${TINYBFT_SRC_DIR}/Message.cc"
        "${TINYBFT_SRC_DIR}/Request.cc"
        "${TINYBFT_SRC_DIR}/Reply.cc"
        "${TINYBFT_SRC_DIR}/Pre_prepare.cc"
        "${TINYBFT_SRC_DIR}/Prepare.cc"
        "${TINYBFT_SRC_DIR}/Commit.cc"
        "${TINYBFT_SRC_DIR}/Checkpoint.cc"
        "${TINYBFT_SRC_DIR}/View_change.cc"
        "${TINYBFT_SRC_DIR}/View_change_ack.cc"
        "${TINYBFT_SRC_DIR}/New_view.cc"
        "${TINYBFT_SRC_DIR}/New_key.cc"
        "${TINYBFT_SRC_DIR}/Status.cc"
        "${TINYBFT_SRC_DIR}/Digest.cc"
        "${TINYBFT_SRC_DIR}/hmac.cc"
        "${TINYBFT_SRC_DIR}/rsa_private_key.cc"
        "${TINYBFT_SRC_DIR}/rsa_public_key.cc"
        "${TINYBFT_SRC_DIR}/Principal.cc"
        "${TINYBFT_SRC_DIR}/Rep_info.cc"
        "${TINYBFT_SRC_DIR}/Req_queue.cc"
        "${TINYBFT_SRC_DIR}/Prepared_cert.cc"
        "${TINYBFT_SRC_DIR}/Pre_prepare_info.cc"
        "${TINYBFT_SRC_DIR}/Meta_data.cc"
        "${TINYBFT_SRC_DIR}/Meta_data_d.cc"
        "${TINYBFT_SRC_DIR}/Meta_data_cert.cc"
        "${TINYBFT_SRC_DIR}/Data.cc"
        "${TINYBFT_SRC_DIR}/Fetch.cc"
        "${TINYBFT_SRC_DIR}/Query_stable.cc"
        "${TINYBFT_SRC_DIR}/Reply_stable.cc"
        "${TINYBFT_SRC_DIR}/NV_info.cc"
        "${TINYBFT_SRC_DIR}/Partition.cc"
        "${TINYBFT_SRC_DIR}/Stable_estimator.cc"
        "${TINYBFT_SRC_DIR}/State.cc"
        "${TINYBFT_SRC_DIR}/Statistics.cc"
        "${TINYBFT_SRC_DIR}/ITimer.cc"
        "${TINYBFT_SRC_DIR}/Time.cc"
        "${TINYBFT_SRC_DIR}/platform.cc"
        "${TINYBFT_SRC_DIR}/new.cc"
        "${TINYBFT_SRC_DIR}/libbyz.cc"
        "${TINYBFT_SRC_DIR}/Log_allocator.cc"
        "${TINYBFT_SRC_DIR}/scratch_allocator.cc"
        "${TINYBFT_SRC_DIR}/agreement_region.cc"
        "${TINYBFT_SRC_DIR}/checkpoint_region.cc"
        "${TINYBFT_SRC_DIR}/special_region.cc"
        "${TINYBFT_SRC_DIR}/checkpoint_log.cc"
        "${TINYBFT_SRC_DIR}/checkpoint_record.cc"
        "${TINYBFT_SRC_DIR}/checkpoint_record_log.cc"
        "${TINYBFT_SRC_DIR}/dsum.cc"
        "${TINYBFT_SRC_DIR}/View_info.cc"
    INCLUDE_DIRS
        "include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${TINYBFT_SRC_DIR}"
    REQUIRES
        mbedtls
        esp_timer
        esp_wifi
        nvs_flash
        spiffs
        json
    PRIV_REQUIRES
        driver
)
```

## 3. idf_component.yml

```yaml
version: "1.0.0"
description: "TinyBFT - Byzantine Fault Tolerance Library"
url: "https://github.com/tinybft/tinybft"
dependencies:
  idf:
    version: ">=5.0.0"
```

## 4. Kconfig Options

```kconfig
menu "TinyBFT Configuration"

    config TINYBFT_ENABLED
        bool "Enable TinyBFT"
        default y
        help
            Enable the TinyBFT BFT consensus library.

    choice TINYBFT_TRANSPORT_CHOICE
        prompt "Transport layer"
        default TINYBFT_TRANSPORT_ESPNOW
        
        config TINYBFT_TRANSPORT_UDP
            bool "UDP"
            help
                Standard UDP transport. Requires WiFi connected to AP.
            
        config TINYBFT_TRANSPORT_ESPNOW
            bool "ESP-NOW"
            help
                ESP-NOW peer-to-peer transport. No AP required.
    endchoice

    config TINYBFT_TRANSPORT
        string
        default "UDP" if TINYBFT_TRANSPORT_UDP
        default "ESP_NOW" if TINYBFT_TRANSPORT_ESPNOW

    config TINYBFT_MAX_MESSAGE_SIZE
        int "Maximum message size (bytes)"
        default 8192
        range 1024 16384
        help
            Maximum size of a single message. Larger messages are fragmented.

    config TINYBFT_NUM_REPLICAS
        int "Number of replicas"
        default 7
        range 4 19
        help
            Total number of replicas. Must satisfy n = 3f + 1.

    config TINYBFT_MAX_NUM_CLIENTS
        int "Maximum concurrent clients"
        default 1
        range 1 10

    config TINYBFT_WINDOW_SIZE
        int "Outstanding requests window"
        default 128
        range 32 256
        help
            Maximum number of concurrent requests in the protocol.

    config TINYBFT_CHECKPOINT_INTERVAL
        int "Checkpoint interval"
        default 128
        range 16 256
        help
            Take checkpoint every N sequence numbers.

    config TINYBFT_WIFI_CHANNEL
        int "WiFi channel (ESP-NOW)"
        default 6
        range 1 14
        help
            WiFi channel for ESP-NOW communication.

    config TINYBFT_FRAG_TIMEOUT_MS
        int "Fragment reassembly timeout (ms)"
        default 5000
        range 1000 30000
        help
            Timeout for reassembling fragmented messages.

endmenu
```

## 5. sdkconfig.defaults

```ini
# TinyBFT defaults
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

# ESP-IDF platform settings
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
```

## 6. Platform Detection

### 6.1 Compile-Time Blocks

```cpp
#ifdef ESP_PLATFORM
    // ESP32-specific includes
    #include <esp_log.h>
    #include <esp_spiffs.h>
    #include <esp_wifi.h>
    #include <esp_now.h>
    #include <esp_timer.h>
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    
    #define TAG "TinyBFT"
    #define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
    #define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#else
    // Linux-specific includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    
    #define LOGI(...) fprintf(stderr, "INFO: " __VA_ARGS__)
    #define LOGE(...) fprintf(stderr, "ERROR: " __VA_ARGS__)
#endif
```

### 6.2 Transport Selection

```cpp
// Compile-time transport selection
#ifdef TINYBFT_TRANSPORT_ESPNOW
    #ifdef ESP_PLATFORM
        transport = std::make_unique<EspNowTransport>();
    #else
        // Stub for Linux compilation
        transport = nullptr;
    #endif
#elif defined(TINYBFT_TRANSPORT_LOOPBACK)
    transport = std::make_unique<LoopbackTransport>();
#else
    transport = std::make_unique<UdpTransport>();
#endif
```

## 7. SPIFFS Configuration

### 7.1 Initialize SPIFFS

```cpp
void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
    }
}
```

### 7.2 File Structure on SPIFFS

```
/spiffs/
├── config              # Peer MAC addresses
├── priv/
│   ├── r0.pem          # Private key
│   ├── r0.pub          # Public key
│   ├── r1.pub          # Peer public keys
│   ├── r2.pub
│   └── ...
└── tinybft.conf        # Additional config
```

### 7.3 Peer Config Format

```
# /spiffs/config
# Format: id MAC_address
0 AA:BB:CC:DD:EE:F0
1 AA:BB:CC:DD:EE:F1
2 AA:BB:CC:DD:EE:F2
```

## 8. ESP-NOW WiFi Setup

```cpp
void init_wifi_espnow(int channel) {
    // Initialize NVS (required by WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_start();
    
    // Disable power save for low latency
    esp_wifi_set_ps(WIFI_PS_NONE);
}
```

## 9. FreeRTOS Integration

### 9.1 Task Priorities

```cpp
// Consensus task: high priority
xTaskCreate(consensus_task, "bft_consensus", 8192, NULL, 10, NULL);

// Network task: medium priority  
xTaskCreate(network_task, "bft_network", 4096, NULL, 8, NULL);

// Application task: lower priority
xTaskCreate(app_task, "bft_app", 4096, NULL, 5, NULL);
```

### 9.2 Timer Usage

```cpp
// View change timer
esp_timer_create_args_t timer_args = {
    .callback = on_view_change_timeout,
    .arg = NULL,
    .name = "view_change"
};
esp_timer_handle_t timer;
esp_timer_create(&timer_args, &timer);
esp_timer_start_periodic(timer, timeout_us);
```

## 10. Memory Configuration

### 10.1 ESP32-C3 Linker Settings

```cmake
# In project's CMakeLists.txt
idf_build_set_property(COMPILE_OPTIONS "-DTINY_BFT=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DMAX_MESSAGE_SIZE=8192" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DMAX_NUM_REPLICAS=7" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DWINDOW_SIZE=128" APPEND)
```

### 10.2 Stack Sizes

| Task | Stack (bytes) | Notes |
|------|---------------|-------|
| Consensus | 8192 | Main protocol loop |
| Network | 4096 | ESP-NOW callbacks |
| Application | 4096 | User logic |
| Timer | 2048 | Timer callbacks |

## 11. Building

### 11.1 Docker Build (Recommended)

```bash
# Build with ESP-IDF Docker image
podman run --rm -v $PWD:/project -w /project \
    espressif/idf:release-v5.5 idf.py build

# Flash
podman run --rm -v $PWD:/project -w /project \
    espressif/idf:release-v5.5 idf.py -p /dev/ttyUSB0 flash monitor
```

### 11.2 Local Build

```bash
source /path/to/esp-idf/export.sh
idf.py set-target esp32c3
idf.py menuconfig  # Configure TinyBFT options
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 12. Running Tests on Hardware

```bash
# Build tests
idf.py build

# Run native tests
idf.py -p /dev/ttyUSB0 flash monitor

# Run specific test
idf.py test "quorum calculation"
```

## 13. ESP32-Specific Considerations

### 13.1 Cycle Counter

```cpp
#if defined(ESP_PLATFORM) && defined(__riscv)
// ESP32-C3 RISC-V
inline uint64_t cycle_count() {
    uint64_t count;
    __asm__("csrr %0, mcycle" : "=r"(count));
    return count;
}
#elif defined(ESP_PLATFORM) && defined(__xtensa__)
// ESP32 Xtensa
inline uint64_t cycle_count() {
    uint32_t count;
    __asm__("rsr.ccount %0" : "=r"(count));
    return count;
}
#endif
```

### 13.2 ESP-NOW Constraints

- **MTU:** 1470 bytes (fragmentation required for larger messages)
- **Max peers:** 20 (enough for n=19 max replicas)
- **Channel:** Must match on all devices
- **Encryption:** Disabled (app-layer auth handles security)

### 13.3 Power Management

```cpp
// Disable WiFi power save for low latency
esp_wifi_set_ps(WIFI_PS_NONE);

// For battery-powered nodes, consider:
// esp_wifi_set_ps(WIFI_PS_MIN_MODEM)
// But this adds latency to ESP-NOW
```

## 14. Integration Checklist

- [ ] Component CMakeLists.txt includes all source files
- [ ] Kconfig options match Linux build options
- [ ] sdkconfig.defaults set correctly
- [ ] SPIFFS partition configured (min 64KB)
- [ ] WiFi initialized before ESP-NOW
- [ ] NVS initialized before WiFi
- [ ] Peer MAC addresses in SPIFFS config
- [ ] Private key in SPIFFS
- [ ] Public keys for all peers in SPIFFS
- [ ] Task priorities set correctly
- [ ] Stack sizes sufficient
- [ ] Memory fits in SRAM
- [ ] Tests pass on hardware

---

## Source Files

- `components/tinybft/CMakeLists.txt` - Component build
- `components/tinybft/Kconfig` - Menuconfig
- `components/tinybft/idf_component.yml` - Component metadata
- `components/tinybft/include/tinybft.h` - Public header
- `src/EspNowTransport.cc` - ESP-NOW implementation
- `src/platform.cc` - Platform-specific code
- `sdkconfig.defaults` - Default ESP-IDF config
