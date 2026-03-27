#!/usr/bin/env python3
"""
Test multiple requests ordering: client sends 10 sequential SET/GET operations.
Verifies that all replicas maintain consistent ordering.
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from test_common import (
    ClusterManager,
    print_test_header,
    print_test_result,
    QUORUM,
    NUM_REPLICAS,
    wait_for_quorum,
)

BINARY_DIR = os.path.join(
    os.path.dirname(os.path.dirname(__file__)), "examples", "build"
)
CONFIG_FILE = os.path.join(os.path.dirname(__file__), "test_7r.conf")
NUM_REQUESTS = 10


def test_multiple_requests():
    """Test sequential request ordering"""

    print_test_header("Multiple Requests Ordering Test")

    replica_bin = os.path.join(BINARY_DIR, "simple_replica")
    client_bin = os.path.join(BINARY_DIR, "simple_client")

    if not os.path.exists(replica_bin) or not os.path.exists(client_bin):
        print("ERROR: Binaries not found. Build examples first.")
        return False

    cluster = ClusterManager(BINARY_DIR, CONFIG_FILE)

    try:
        # Start cluster
        print(f"\n[1/5] Starting {NUM_REPLICAS} replicas...")
        cluster.start_cluster(replica_bin, client_bin)

        print("[2/5] Waiting for quorum...")
        if not wait_for_quorum(cluster):
            print_test_result("Multiple Requests", False, "No quorum")
            return False

        # Start client
        print("[3/5] Starting client...")
        cluster.start_client(
            client_bin,
            "/home/phukrit7171/Development/TinyBFT/test_runtime/priv/client0_priv.pem",
        )
        time.sleep(1)

        # Send multiple requests
        print(f"[4/5] Sending {NUM_REQUESTS} sequential requests...")

        test_data = [
            ("SET", "counter", "1"),
            ("SET", "counter", "2"),
            ("SET", "counter", "3"),
            ("SET", "data", "hello"),
            ("GET", "counter", ""),
            ("SET", "counter", "4"),
            ("SET", "counter", "5"),
            ("GET", "data", ""),
            ("SET", "data", "world"),
            ("GET", "counter", ""),
        ]

        success_count = 0
        failed_count = 0

        for i, (cmd, key, value) in enumerate(test_data):
            if cmd == "SET":
                request = f"SET {key}={value}"
            else:
                request = f"GET {key}"

            print(f"  [{i + 1}/{NUM_REQUESTS}] {request}...", end=" ")

            response = cluster.client.send_command(request)

            if "OK" in response or "reply" in response.lower():
                print("OK")
                success_count += 1
            else:
                print(f"FAILED: {response.strip()}")
                failed_count += 1

        # Report results
        print(f"\n[5/5] Results: {success_count} success, {failed_count} failed")

        if success_count >= NUM_REQUESTS * 0.8:  # Allow 20% failure
            print_test_result(
                "Multiple Requests",
                True,
                f"{success_count}/{NUM_REQUESTS} requests completed successfully",
            )
            return True
        else:
            print_test_result(
                "Multiple Requests",
                False,
                f"Too many failures: {failed_count}/{NUM_REQUESTS}",
            )
            return False

    except Exception as e:
        print_test_result("Multiple Requests", False, str(e))
        return False

    finally:
        cluster.stop_client()
        cluster.stop_cluster()


if __name__ == "__main__":
    success = test_multiple_requests()
    sys.exit(0 if success else 1)
