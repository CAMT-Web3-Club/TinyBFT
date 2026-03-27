# Testing Framework - TinyBFT

## Frameworks
- **Primary**: Unity (ESP-IDF native test runner).
- **Runner**: PlatformIO for automated local execution and management.

## Test Locations
- **Core Library Tests**: `components/tinybft/test/`
- **Application Tests**: `test/`

## Testing Strategy
- **Hardware-in-the-Loop (HIL)**: Tests are designed to run on physical ESP32-C3 hardware.
- **Mocking**: Used for transport and timing where needed to isolate consensus logic.
- **Priority Areas**:
  - **P0**: Consensus Protocol (Pre-prepare, Prepare, Commit phases).
  - **P0**: Quorum Verification (Quorum intersection and counts).
  - **P0**: Safety Invariants (No double-proposes, monotonic counters).
  - **P1**: View Changes (Primary rotation and failure detection).

## Execution Commands
### PlatformIO
```bash
# Run all tests on ESP32-C3
pio test -e esp32c3
```

### ESP-IDF (Native)
```bash
# Build and run tests using idf.py
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
