# TinyBFT Roadmap

## Milestone 1: Stabilization & Foundation (Current)
- Initialize GSD tracking framework.
- Investigate and fix ESP32-C3 test suite failures.
- Verify MbedTLS correctly utilizes ESP-IDF's internal variant.

## Milestone 2: PBFT memory optimizations
- Memory profiling.
- Protocol size optimizations (BLOCK_SIZE, MAX_MESSAGE_SIZE).

## Milestone 3: ESP-NOW Tuning
- Refine multi-part message fragmentation over ESP-NOW.
