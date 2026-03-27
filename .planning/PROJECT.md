# Project: TinyBFT
# Objective: Develop a PBFT optimization library for embedded systems on ESP32-C3 under ESP-IDF.

## Context
TinyBFT is based on MIT's PBFT `libbyz`. The goal is to optimize it for resource-constrained devices, taking into account memory limits (modifying sizes, MbedTLS integrations, block sizes) and exploiting ESP-NOW for transport on ESP32-C3.

## Environment
- Hardware: ESP32-C3
- Framework: ESP-IDF (via PlatformIO or native idf.py)
- Language: C/C++ (C11/C++14)
- Network: ESP-NOW

## Current Status
- Ported basic logic from `libbyz`.
- Migrated crypto from `sfslite` to MbedTLS (using ESP-IDF native component).
- Tests on hardware are failing ("device reports readiness to read but returned no data").

## Project Rules
- Follow guidelines in AGENTS.md.
- Run tests via `pio test -e esp32c3`.
- Exclusively use `mbedtls` from the ESP-IDF component framework on hardware.
