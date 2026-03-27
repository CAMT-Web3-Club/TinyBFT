# Tech Stack - TinyBFT

## Core Technologies
- **Framework**: ESP-IDF (release-v5.5)
- **Secondary Framework**: PlatformIO (for build and test management)
- **Languages**: 
  - C11 (`-std=c11`)
  - C++14 (`-std=c++14`)
- **Build System**: 
  - CMake (minimum 3.18.4)
  - PlatformIO (`pio`)

## Hardware Targets
- **Primary**: ESP32-C3 (RISC-V)
- **Development**: Any ESP32 compatible with ESP-IDF v5.x

## Libraries & Dependencies
- **MbedTLS**: Used for SHA256 hashing and HMAC generation.
- **Unity**: Used for unit testing (native ESP-IDF runner).
- **ESP-NOW**: Primary low-level wireless transport protocol.

## Tooling
- **Compiler**: `riscv32-esp-elf-gcc` / `riscv32-esp-elf-g++`
- **Flashing**: `esptool.py`
- **Containerization**: Podman/Docker (using `espressif/idf` images)
- **Environment Management**: Nix (via `flake.nix`)
