# AGENTS.md - Guidelines for Agentic Coding in TinyBFT

This document provides guidelines for agents operating in the TinyBFT codebase.

## 1. Build Commands

### Linux Build

```bash
# Create and enter build directory
mkdir -p build && cd build

# Configure with default settings (UDP transport)
cmake ..

# Or with specific options
cmake .. -DTINYBFT_TRANSPORT=UDP        # UDP transport (default)
cmake .. -DTINYBFT_TRANSPORT=LOOPBACK   # Loopback transport for testing
cmake .. -DDISABLE_MULTICAST=1          # Use unicast instead of multicast
cmake .. -DTINY_BFT=1                  # Enable TinyBFT memory optimizations
cmake .. -DMAX_MESSAGE_SIZE=8192       # Custom max message size
cmake .. -DMAX_NUM_REPLICAS=7          # Number of replicas (default: 32)

# Build
make -j$(nproc)
```

### ESP32-C3 Build (ESP-IDF)

```bash
# Set target
idf.py set-target esp32c3

# Configure via menuconfig
idf.py menuconfig

# Build
idf.py build
```

### Running Tests

```bash
# Build the library first
cd build && cmake .. && make -j4

# Build examples
cd examples/build && cmake .. && make -j4

# Run full consensus test (1 client + 4 replicas)
python3 test_runtime/run_full_test.py

# Run simple client test
./examples/build/simple_client test_runtime/test.conf test_runtime/priv/client0.pem
```

### Build Options Reference

| Option | Values | Default | Description |
|--------|--------|---------|--------------|
| `TINYBFT_TRANSPORT` | UDP, LOOPBACK, ESP_NOW | UDP | Transport layer |
| `DISABLE_MULTICAST` | 0, 1 | 0 | Use unicast instead of multicast |
| `MAX_MESSAGE_SIZE` | 1024-16384 | 16384 | Max message size in bytes |
| `MAX_NUM_REPLICAS` | 4-19 | 32 | Number of replicas |
| `WINDOW_SIZE` | 32-256 | 256 | Outstanding requests window |
| `TINY_BFT` | 0, 1 | 0 | Enable memory optimizations |

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
  2. System headers (`<cstdint>`, `<sys/socket.h>`)
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
│   ├── Transport.cc/h       # Transport abstraction
│   ├── EspNowTransport.cc/h # ESP-NOW implementation
│   ├── Fragmentation.cc/h  # Message fragmentation
│   └── ...
├── include/               # Public headers
├── examples/              # Example applications
│   ├── simple_client.cc   # Basic client
│   ├── simple_replica.cc  # Basic replica with KV store
│   └── test_client.cc     # Test client with SET/GET
├── test_runtime/          # Test configuration
│   ├── test.conf          # Main config
│   ├── priv/             # Private keys (gitignored)
│   └── run_full_test.py   # Python test runner
├── components/tinybft/    # ESP-IDF component
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

### Running a Single Test

The Python test runner starts 4 replicas and 1 client automatically:

```bash
# Full test with all SET/GET operations
python3 test_runtime/run_full_test.py

# Manual testing
./examples/build/simple_replica test_runtime/test.conf test_runtime/priv/r0.pem 5679 &
./examples/build/simple_replica test_runtime/test.conf test_runtime/priv/r1.pem 5680 &
./examples/build/simple_client test_runtime/test.conf test_runtime/priv/client0.pem
```

### Manual Test Commands

```bash
# Start replica
./examples/build/simple_replica <config> <private_key> <port>

# Start client  
./examples/build/simple_client <config> <private_key>

# Client sends commands like:
# - SET key=value
# - GET key
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

Use compile definitions for platform-specific code:

```cpp
#ifdef ESP_PLATFORM
    // ESP32-C3 specific
    #include <esp_now.h>
#else
    // Linux specific
    #include <sys/socket.h>
#endif
```

---

## 8. Security Considerations

- Never commit private keys (they are in `.gitignore`)
- Use mbedTLS for cryptographic operations
- Validate all message sizes before processing
- Check authentication before processing messages
