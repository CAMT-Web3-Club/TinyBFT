# TinyBFT Requirements

## Functional Requirements
- Complete PBFT consensus protocol implementation (Pre-Prepare, Prepare, Commit).
- ESP-NOW transport capability.
- Hardware-backed MbedTLS cryptographic verification (RSA signatures and UMACs).

## Non-Functional Requirements
- Must compile successfully under ESP-IDF 5.x.
- Keep RAM overhead minimal to run on ESP32-C3 (which has ~400KB static RAM).
- Gracefully handle timeouts under unreliable ESP-NOW wireless links.
