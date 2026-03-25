#!/usr/bin/env python3
"""
Test fault tolerance: kill 2 replicas (simulating Byzantine faults),
verify system continues with remaining 5 replicas.
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from test_common import (
    ClusterManager, print_test_header, print_test_result,
    QUORUM, NUM_REPLICAS, NUM_FAULTY, wait_for_quorum
)

BINARY_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples", "build")
CONFIG_FILE = os.path.join(os.path.dirname(__file__), "test_7r.conf")


def test_fault_tolerance():
    """Test system survives with f=2 faulty replicas"""
    
    print_test_header("Fault Tolerance Test")
    
    replica_bin = os.path.join(BINARY_DIR, "simple_replica")
    client_bin = os.path.join(BINARY_DIR, "simple_client")
    
    if not os.path.exists(replica_bin) or not os.path.exists(client_bin):
        print("ERROR: Binaries not found.")
        return False
    
    cluster = ClusterManager(BINARY_DIR, CONFIG_FILE)
    
    try:
        # Start all 7 replicas
        print(f"\n[1/6] Starting {NUM_REPLICAS} replicas...")
        cluster.start_cluster(replica_bin, client_bin)
        
        print("[2/6] Waiting for initial quorum...")
        if not wait_for_quorum(cluster, timeout=15):
            print_test_result("Fault Tolerance", False, "Initial quorum failed")
            return False
        
        live_initial = cluster.get_live_replicas()
        print(f"       Initial live replicas: {live_initial}")
        
        # Start client
        print("[3/6] Starting client...")
        cluster.start_client(client_bin, "priv/client0.pem")
        time.sleep(1)
        
        # Send initial request
        print("[4/6] Sending initial SET request...")
        response = cluster.client.send_command("SET before=kill")
        print(f"       Response: {response.strip()}")
        
        # Kill 2 replicas (simulate Byzantine faults)
        print(f"[5/6] Killing {NUM_FAULTY} replicas (simulating faults)...")
        kill_ids = [0, 1]  # Kill first 2 replicas
        cluster.kill_replicas(kill_ids)
        time.sleep(1)
        
        # Check remaining replicas
        live_after_kill = cluster.get_live_replicas()
        remaining = len(live_after_kill)
        print(f"       Remaining replicas: {remaining}")
        
        # Verify still have quorum (5 >= quorum)
        if remaining < QUORUM:
            print_test_result("Fault Tolerance", False,
                f"Only {remaining} replicas, need quorum={QUORUM}")
            return False
        
        print(f"       Quorum maintained: {remaining} >= {QUORUM}")
        
        # Send request after killing replicas
        print("[6/6] Sending request after fault injection...")
        response = cluster.client.send_command("SET after=kill value=42")
        
        if "OK" in response or "reply" in response.lower():
            print_test_result("Fault Tolerance", True,
                f"System continues with {remaining} replicas (quorum={QUORUM})")
            return True
        else:
            print_test_result("Fault Tolerance", False,
                f"Request failed: {response.strip()}")
            return False
            
    except Exception as e:
        print_test_result("Fault Tolerance", False, str(e))
        return False
        
    finally:
        cluster.stop_client()
        cluster.stop_cluster()


if __name__ == "__main__":
    success = test_fault_tolerance()
    sys.exit(0 if success else 1)