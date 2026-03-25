#!/usr/bin/env python3
"""
Simple test runner for TinyBFT consensus
"""

import subprocess
import time
import os
import signal
import sys

PROJECT_DIR = "/home/phukrit7171/Development/TinyBFT"
CONF = f"{PROJECT_DIR}/test_runtime/test.conf"
KEYS = f"{PROJECT_DIR}/test_runtime/priv"


def run_cmd(cmd, timeout=30):
    """Run command and return output"""
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=PROJECT_DIR,
        )
        return result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return "TIMEOUT"
    except Exception as e:
        return f"ERROR: {e}"


def main():
    print("=== TinyBFT Consensus Test ===")

    # Kill existing
    run_cmd("pkill -9 -f simple 2>/dev/null")
    time.sleep(1)

    # Build
    print("Building...")
    run_cmd("cd build && cmake .. -DDISABLE_MULTICAST=1 && make -j4", timeout=120)
    run_cmd("cd examples/build && cmake .. && make -j4", timeout=60)

    # Start replicas
    replicas = []
    for i in range(4):
        port = 5679 + i
        key = f"{KEYS}/r{i}.pem"
        cmd = f"./examples/build/simple_replica {CONF} {key} {port}"
        print(f"Starting replica {i} on port {port}...")
        proc = subprocess.Popen(
            cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=PROJECT_DIR,
        )
        replicas.append(proc)
        time.sleep(2)

    # Check replicas
    print("\nChecking replicas...")
    for i, proc in enumerate(replicas):
        if proc.poll() is None:
            print(f"  Replica {i}: running")
        else:
            print(f"  Replica {i}: exited early")

    time.sleep(3)

    # Start client
    print("\nStarting client...")
    client_cmd = f"./examples/build/simple_client {CONF} {KEYS}/client0.pem"
    client = subprocess.Popen(
        client_cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=PROJECT_DIR,
    )

    # Wait for client with timeout
    print("Waiting for client (30s timeout)...")
    try:
        stdout, stderr = client.communicate(timeout=30)
        print("\nClient output:")
        print(stdout.decode() if isinstance(stdout, bytes) else stdout)
        print(stderr.decode() if isinstance(stderr, bytes) else stderr)
    except subprocess.TimeoutExpired:
        print("Client TIMEOUT - killing...")
        client.kill()

    # Cleanup
    print("\nCleaning up...")
    for proc in replicas:
        proc.kill()
    run_cmd("pkill -9 -f simple 2>/dev/null")

    print("\n=== Test Complete ===")


if __name__ == "__main__":
    main()
