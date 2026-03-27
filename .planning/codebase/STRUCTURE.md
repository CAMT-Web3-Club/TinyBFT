# Project Structure - TinyBFT

## Directory Map
```text
TinyBFT/
├── src/                    # Library source code (libbyzea)
│   ├── Node.cc/h           # Base communication node
│   ├── Replica.cc/h        # Core consensus logic
│   ├── Client.cc/h         # Client request logic
│   ├── Message.cc/h        # Message base and parsing
│   ├── Transport.cc/h      # Transport abstraction
│   └── ...                 # Protocol phase implementations (Prepare.cc, etc.)
├── include/               # Public headers for library users
├── components/            # ESP-IDF Components
│   ├── tinybft/           # The TinyBFT library as a component
│   │   └── test/          # ESP-IDF native tests (Unity)
│   └── main/              # Application logic component
├── main/                  # ESP-IDF project main source
│   ├── main.cc            # App entry point
│   └── CMakeLists.txt     # Build config
├── test/                  # Additional standalone tests
├── docs/                  # Design specifications and documentation
├── platformio.ini         # PlatformIO configuration
├── CMakeLists.txt         # Root ESP-IDF build configuration
└── .planning/             # Project state and mapping (this folder)
```

## Module Responsibilities
- **libbyzea (src/)**: Implements the PBFT protocol independently of the hardware where possible.
- **TinyBFT Component**: Wraps the library for the ESP-IDF build system and provides ESP32-specific drivers (like ESP-NOW).
- **Test Suite**: Located in `components/tinybft/test/`, verified to run on ESP32-C3 hardware.
