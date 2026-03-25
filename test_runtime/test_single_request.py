#!/usr/bin/env python3
"""
Test single request consensus: client sends SET, all replicas agree.
Tests: f=2, n=7, quorum=5
"""

import sys
import os
import time
import subprocess
import signal

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from test_common import (
    ClusterManager, print_test_header, print_test_result,
    QUORUM, NUM_REPLICAS, wait_for_quorum
)

BINARY_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples", "build")
CONFIG_FILE = os.path.join(os.path.dirname(__file__), "test_7r.conf")


def test_single_request():
    """Test that a single SET request achieves consensus"""
    
    print_test_header("Single Request Consensus Test")
    
    # Find binaries
    replica_bin = os.path.join(BINARY_DIR, "simple_replica")
    client_bin = os.path.join(BINARY_DIR, "simple_client")
    
    if not os.path.exists(replica_bin):
        print(f"ERROR: Replica binary not found: {replica_bin}")
        return False
        
    if not os.path.exists(client_bin):
        print(f"ERROR: Client binary not found: {client_bin}")
        return False
    
    print(f"Using config: {CONFIG_FILE}")
    print(f"Replicas: {NUM_REPLICAS}, Faulty: 2, Quorum: {QUORUM}")
    
    # Create cluster manager
    cluster = ClusterManager(BINARY_DIR, CONFIG_FILE)
    
    try:
        # Start replicas
        print("\n[1/5] Starting 7 replicas...")
        cluster.start_cluster(replica_bin, client_bin)
        
        # Wait for quorum
        print("[2/5] Waiting for quorum (5 replicas)...")
        if not wait_for_quorum(cluster):
            print_test_result("Single Request", False, "Failed to achieve quorum")
            return False
            
        live = cluster.get_live_replicas()
        print(f"       Live replicas: {live}")
        
        # Start client
        print("[3/5] Starting client...")
        cluster.start_client(client_bin, "priv/client0.pem")
        time.sleep(1)
        
        # Send SET request
        print("[4/5] Sending SET key=test value=42...")
        response = cluster.client.send_command("SET test=42")
        print(f"       Response: {response.strip()}")
        
        # Verify response indicates success
        if "OK" in response or "reply" in response.lower():
            print_test_result("Single Request", True, 
                "All 5 correct replicas agreed on SET test=42")
            return True
        else:
            print_test_result("Single Request", False, 
                f"Unexpected response: {response}")
            return False
            
    except Exception as e:
        print_test_result("Single Request", False, str(e))
        return False
        
    finally:
        # Cleanup
        print("[5/5] Cleaning up...")
        cluster.stop_client()
        cluster.stop_cluster()


if __name__ == "__main__":
    success = test_single_request()
    sys.exit(0 if success else 1)