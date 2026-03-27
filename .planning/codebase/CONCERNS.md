# Concerns & Risks - TinyBFT

## Resource Constraints
- **Memory (RAM)**: TinyBFT is memory-intensive (logs, state, message buffers). With only 320KB of RAM on the ESP32-C3, strict memory management is required.
- **Fragmentation**: Large messages (up to 16KB) may cause heap fragmentation or require careful pre-allocation.

## Network Reliability
- **ESP-NOW Latency**: While low-latency, ESP-NOW is unconfirmed and could lose packets. The PBFT protocol handles this but requires careful tuning of timeouts.
- **Node Discovery**: Current configuration might be static; dynamic joining of new replicas needs further implementation/testing.

## Security
- **Key Management**: Ensuring private keys are handled securely within the ESP32's protected memory (NVS or Efuses).
- **MAC Performance**: Generating and verifying multiple MACs per message could impact latency; optimizations like MAC-groups or digital signatures should be evaluated if needed.

## Protocol Edge Cases
- **View Change Stability**: High network churn could lead to "view-change loops" where nodes never reach consensus.
- **Fetch Logic**: Syncing large state snapshots across slow wireless links could block the replica's progress if not handled asynchronously.

## Maintenance
- **Native Test Runner**: Linker issues with Unity observed and fixed; keeping the PlatformIO and ESP-IDF build environments in sync is a priority.
