# Integrations - TinyBFT

## Internal Components
- **Consensus Library (libbyzea)**: The core BFT protocol implementation.
- **ESP-NOW Transport**: Integration with ESP32-C3 wireless stack via ESP-NOW.

## External Interfaces
- **Client API**: Provides `Client` class for applications to submit requests and receive replies.
- **Replica Interface**: Provides `Replica` class for implementing service logic.

## System Integrations
- **MbedTLS**: Deep integration for cryptographic safety (hashing, MACs).
- **ESP-IDF NVS/SPIFFS**: (Planned/Used) for persistent storage of keys and configuration.
- **PlatformIO**: Integration for automated testing and CI/CD-like workflows locally.

## Development Environment
- **Nix**: Provides a reproducible development environment including specific MbedTLS versions and build tools.
- **Podman/Docker**: Used for hermetic builds using official Espressif images.
