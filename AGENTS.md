# AGENTS.md - Guidelines for Agentic Coding in TinyBFT

This document provides guidelines for agents operating in the TinyBFT codebase.

## 1. Build Commands (ESP-IDF)

### Build (Container)

```bash
# Build using Docker/Podman container
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py build

# Set target
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py set-target esp32c3

# Configure via menuconfig
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py menuconfig
```

### Build (PlatformIO)

```bash
# Build using PlatformIO
pio run

# Run tests using PlatformIO
pio test -e esp32c3
```

### Flash (Host)

```bash
# Flash using esptool on host (not in container)
esptool --chip esp32c3 --port /dev/ttyACM0 --baud 460800 write-flash \
  --flash-mode dio --flash-size 4MB --flash-freq 80m \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/tinybft_app.bin

# Or reset only (no flash)
esptool --chip esp32c3 --port /dev/ttyACM0 --baud 460800 run
```

### Monitor (Host)

```bash
# Monitor using screen on host
screen /dev/ttyACM0 115200

# Or capture output to file
timeout 30 cat /dev/ttyACM0 > output.txt
```

---

## 2. Code Style Guidelines

### Language Standards

- **C**: C11 (`-std=c11`)
- **C++**: C++14 (`-std=c++14`)
- **CMake**: Minimum 3.18.4

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `class Node`, `class Message` |
| Functions | PascalCase | `void SendMessage()`, `int GetSize()` |
| Variables | snake_case | `int max_size`, `bool is_ready` |
| Constants | SCREAMING_SNAKE_CASE | `MAX_MESSAGE_SIZE`, `BLOCK_SIZE` |
| Enum values | PascalCase | `TransportType::UDP` |
| Files | snake_case | `node.cc`, `message.h` |
| Member variables | snake_case_ with trailing `_` | `node_id_`, `max_size_` |
| Static variables | s_ prefix | `s_instance` |

### Formatting

- **Indent**: 4 spaces (no tabs)
- **Line length**: 100 characters max
- **Braces**: Allman style (K&R for functions)
  ```cpp
  if (condition)
  {
      do_something();
  }
  ```
- **Namespaces**: Use `libbyzea` namespace for all library code
- **Include order**:
  1. Library headers (libbyzea)
  2. System headers (`<cstdint>`, `<esp_now.h>`)
  3. Third-party (`<mbedtls/...>`)

### Type Guidelines

| Type | Usage |
|------|-------|
| `int` | General integers (prefer for compatibility) |
| `size_t` | Sizes, counts |
| `uint32_t`, `uint16_t` | Fixed-width for network/protocol |
| `bool` | Boolean values |
| `std::string` | C++ strings |
| `std::unique_ptr<T>` | Smart pointers for ownership |
| `std::shared_ptr<T>` | Smart pointers for shared ownership |

### Error Handling

- **Return codes**: Use `int` return with 0 for success, negative for error
- **Assertions**: Use `th_assert()` for internal invariants
- **Logging**: Use `fprintf(stderr, ...)` for errors
- **Exceptions**: Do not use C++ exceptions (not supported by all embedded targets)

### Memory Management

- Use `new`/`delete` for C++ objects
- Use `malloc`/`free` for C code
- Prefer static allocation when possible (`STATIC_LOG_ALLOCATOR`)
- Always free resources in destructors

### Comments

- Use comments to explain **why**, not **what**
- No comments for obvious code
- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Document public API in headers

---

## 3. Project Structure

```
TinyBFT/
├── src/                    # Library source code
│   ├── Node.cc/h           # Main consensus node
│   ├── Client.cc/h         # Client implementation
│   ├── Replica.cc/h        # Replica implementation
│   ├── Message.cc/h        # Message base class
│   ├── Request.cc/h        # Client request
│   ├── Reply.cc/h          # Client reply
│   ├── Transport.cc/h      # Transport abstraction
│   ├── EspNowTransport.cc/h # ESP-NOW implementation
│   ├── Fragmentation.cc/h  # Message fragmentation
│   └── ...
├── include/               # Public headers
├── examples/              # Example applications
│   ├── simple_client.cc   # Basic client
│   ├── simple_replica.cc  # Basic replica with KV store
│   └── test_client.cc     # Test client with SET/GET
├── components/tinybft/    # ESP-IDF library component
│   └── test/              # ESP-IDF native tests (Unity)
│       ├── CMakeLists.txt
│       ├── test_consensus.cc
│       ├── test_safety.cc
│       └── test_view_change.cc
└── build/                 # Build output (gitignored)
```

---

## 4. Important Patterns

### Transport Abstraction

The codebase uses a transport abstraction layer. New transports should:

1. Inherit from `Transport` base class
2. Implement all pure virtual methods
3. Register in `Transport::create()` factory

```cpp
class MyTransport : public Transport {
public:
    void init() override;
    void send(Message* m, int dest_id) override;
    Message* recv() override;
    void add_peer(int id, const uint8_t* addr) override;
    bool is_ready() override;
    size_t max_payload_size() const override;
    std::string peer_addr_string(int id) const override;
    TransportType type() const override;
};
```

### Adding New Source Files

When adding new `.cc` or `.cpp` files:

1. Add to `CMakeLists.txt` in `target_sources(byzea PRIVATE ...)`
2. Add corresponding header to `include/` if public
3. Update this file if adding new build options

### Message Types

All message types should:
- Inherit from `Message` base class
- Use `Message_rep` header structure
- Implement `convert()` static method for parsing
- Use `mark_verified()`/`mark_invalid()` for authentication

---

## 5. Testing Guidelines

TinyBFT uses ESP-IDF native tests on hardware for validation.

### Running ESP-IDF Native Tests (Hardware)

```bash
# Build library and tests
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py build

# Run tests on hardware
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py -p /dev/ttyUSB0 flash monitor

# Or run specific test case
podman run --rm -v $PWD:/project -w /project espressif/idf:release-v5.5 idf.py test "quorum calculation"
```

### Test Configuration (f=2, n=7)

| Parameter | Value | Description |
|-----------|-------|-------------|
| n | 7 | Total replicas |
| f | 2 | Maximum faulty |
| quorum | 5 | 2f+1 required |

### Test Priorities

| Priority | Test Area | Location | Framework |
|---------|-----------|----------|-----------|
| P0 | Consensus Protocol | components/tinybft/test/ | Unity |
| P0 | Quorum Verification | components/tinybft/test/ | Unity |
| P0 | Safety Invariants | components/tinybft/test/ | Unity |
| P1 | View Changes | components/tinybft/test/ | Unity |

### Test Directory Structure

```
TinyBFT/
├── components/tinybft/test/       # ESP-IDF native tests (Unity)
│   ├── CMakeLists.txt
│   ├── test_consensus.cc
│   ├── test_safety.cc
│   └── test_view_change.cc
```

---

## 6. Key Files Reference

| File | Purpose |
|------|---------|
| `src/Node.cc` | Main node logic, send/recv |
| `src/Replica.cc` | Replica consensus logic |
| `src/Client.cc` | Client request handling |
| `src/Message.h` | Base message class |
| `src/Transport.h` | Transport interface |
| `CMakeLists.txt` | Build configuration |

---

## 7. Common Tasks

### Adding a New Message Type

1. Create `src/NewMessage.cc` and `src/NewMessage.h`
2. Add to `CMakeLists.txt`
3. Implement message parsing in `Message.cc`
4. Add handler in `Replica.cc`

### Modifying Consensus Logic

Consensus is implemented in:
- `src/Replica.cc` - Main replica logic
- `src/Pre_prepare.cc` - Pre-prepare phase
- `src/Prepare.cc` - Prepare certificate
- `src/Commit.cc` - Commit certificate

### Platform-Specific Code

Use compile definitions for ESP-IDF specific code:

```cpp
#ifdef ESP_PLATFORM
    // ESP32-C3 specific
    #include <esp_now.h>
#endif
```

---

## 8. Security Considerations

- Never commit private keys (they are in `.gitignore`)
- Use mbedTLS for cryptographic operations
- Validate all message sizes before processing
- Check authentication before processing messages
