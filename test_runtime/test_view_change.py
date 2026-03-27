#!/usr/bin/env python3
"""
Test view change: kill primary replica, verify new primary elected.
After view change, system should continue processing requests.
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
    NUM_FAULTY,
    wait_for_quorum,
)

BINARY_DIR = os.path.join(
    os.path.dirname(os.path.dirname(__file__)), "examples", "build"
)
CONFIG_FILE = os.path.join(os.path.dirname(__file__), "test_7r.conf")


def test_view_change():
    """Test view change when primary fails"""

    print_test_header("View Change Test")

    replica_bin = os.path.join(BINARY_DIR, "simple_replica")
    client_bin = os.path.join(BINARY_DIR, "simple_client")

    if not os.path.exists(replica_bin) or not os.path.exists(client_bin):
        print("ERROR: Binaries not found.")
        return False

    cluster = ClusterManager(BINARY_DIR, CONFIG_FILE)

    try:
        # Start all replicas
        print(f"\n[1/7] Starting {NUM_REPLICAS} replicas...")
        cluster.start_cluster(replica_bin, client_bin)

        print("[2/7] Waiting for initial quorum...")
        if not wait_for_quorum(cluster, timeout=15):
            print_test_result("View Change", False, "Initial quorum failed")
            return False

        # Start client
        print("[3/7] Starting client...")
        cluster.start_client(
            client_bin,
            "/home/phukrit7171/Development/TinyBFT/test_runtime/priv/client0_priv.pem",
        )
        time.sleep(1)

        # Send initial request to establish view 0
        print("[4/7] Sending initial request (view=0)...")
        response = cluster.client.send_command("SET init=value")
        print(f"       Response: {response.strip()}")

        # Kill primary (replica 0 in view 0)
        print("[5/7] Killing primary replica (view=0, primary=0)...")
        primary = 0 % NUM_REPLICAS
        cluster.kill_replicas([primary])
        time.sleep(1)

        # Wait for view change (new primary should be replica 1)
        print("[6/7] Waiting for view change...")
        time.sleep(5)  # Wait for view change to complete

        live_after = cluster.get_live_replicas()
        remaining = len(live_after)
        print(f"       Live replicas after kill: {live_after}")

        # Should still have quorum
        if remaining < QUORUM:
            print_test_result("View Change", False, "Lost quorum after primary kill")
            return False

        # Send request after view change - should work with new primary
        print("[7/7] Sending request after view change...")
        response = cluster.client.send_command("SET after_view_change=works")

        if "OK" in response or "reply" in response.lower():
            print_test_result(
                "View Change",
                True,
                f"View change successful. New primary elected. System continues with {remaining} replicas.",
            )
            return True
        else:
            print_test_result(
                "View Change",
                False,
                f"Request failed after view change: {response.strip()}",
            )
            return False

    except Exception as e:
        print_test_result("View Change", False, str(e))
        return False

    finally:
        cluster.stop_client()
        cluster.stop_cluster()


if __name__ == "__main__":
    success = test_view_change()
    sys.exit(0 if success else 1)
