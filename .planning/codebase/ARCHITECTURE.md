# Architecture - TinyBFT

## Overview
TinyBFT is a Practical Byzantine Fault Tolerance (PBFT) implementation optimized for embedded systems (ESP32-C3). It follows the classical 3-phase consensus protocol to reach agreement among a set of replicas.

## Core Components
- **Replica**: The main consensus node that processes requests, participates in agreement, and manages state.
- **Node**: A base class for both Replicas and Clients, handling network identification and message transmission.
- **State Management**: Uses a Copy-On-Write (COW) mechanism for efficient checkpointing and recovery of the application state.
- **Transport Layer**: An abstraction layer that decouples the consensus logic from the underlying network protocol (ESP-NOW, UDP, etc.).

## Consensus Protocol
1. **Pre-prepare**: The primary replica assigns a sequence number to a client request and broadcasts it.
2. **Prepare**: Replicas verify the pre-prepare and broadcast their agreement. A "prepared certificate" is formed when 2f+1 prepares are received.
3. **Commit**: Replicas broadcast that they have a prepared certificate. A "committed certificate" is formed when 2f+1 commits are received.
4. **Execution**: Once a request is committed, it is executed against the application state.

## Safety & Liveness
- **Safety**: Guaranteed as long as no more than `f` replicas are faulty, where `n >= 3f + 1`.
- **View Changes**: Triggered by backups when they suspect the primary has failed, ensuring liveness by rotating the primary role.
- **Checkpoints**: Periodic state captures every `k` requests to truncate logs and allow new replicas to synchronize.
