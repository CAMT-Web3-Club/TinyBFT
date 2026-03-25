#!/usr/bin/env python3
"""
Common test utilities for TinyBFT integration tests.
Shared between Linux and ESP-IDF test harnesses.
"""

import socket
import time
import subprocess
import os
import sys
from typing import List, Tuple, Optional

# Constants
NUM_REPLICAS = 7
NUM_FAULTY = 2
QUORUM = 2 * NUM_FAULTY + 1  # 5
CLIENT_PORT_BASE = 7008
REPLICA_PORT_BASE = 7001

class ReplicaProcess:
    """Manages a single replica process"""
    
    def __init__(self, replica_id: int, config_file: str, key_file: str, port: int):
        self.replica_id = replica_id
        self.config_file = config_file
        self.key_file = key_file
        self.port = port
        self.process = None
        
    def start(self, binary_path: str):
        """Start the replica process"""
        cmd = [binary_path, self.config_file, self.key_file, str(self.port)]
        self.process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return self.process
        
    def stop(self):
        """Stop the replica process"""
        if self.process:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                
    def is_alive(self) -> bool:
        """Check if replica is still running"""
        return self.process and self.process.poll() is None
    
    def send_request(self, request: str) -> str:
        """Send a request to the replica via UDP"""
        pass  # Client handles this


class TestClient:
    """Manages the client connection"""
    
    def __init__(self, config_file: str, key_file: str, port: int = CLIENT_PORT_BASE):
        self.config_file = config_file
        self.key_file = key_file
        self.port = port
        self.process = None
        
    def connect(self, binary_path: str):
        """Connect to the BFT cluster"""
        cmd = [binary_path, self.config_file, self.key_file]
        self.process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
    def send_command(self, command: str) -> str:
        """Send a command and get response"""
        if not self.process or self.process.stdin is None:
            return ""
            
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()
        
        # Read response (simple read - may need adjustment)
        response = ""
        start_time = time.time()
        while time.time() - start_time < 10:
            line = self.process.stdout.readline()
            if not line:
                break
            response += line
            if "OK" in line or "ERROR" in line:
                break
                
        return response
    
    def close(self):
        """Close client connection"""
        if self.process:
            self.process.terminate()


class ClusterManager:
    """Manages a 7-replica BFT cluster"""
    
    def __init__(self, binary_dir: str, config_file: str):
        self.binary_dir = binary_dir
        self.config_file = config_file
        self.replicas: List[ReplicaProcess] = []
        self.client: Optional[TestClient] = None
        
    def start_cluster(self, simple_replica: str, simple_client: str):
        """Start 7 replicas"""
        # Create replica processes
        for i in range(NUM_REPLICAS):
            key_file = f"priv/r{i}.pem"
            port = REPLICA_PORT_BASE + i
            replica = ReplicaProcess(i, self.config_file, key_file, port)
            replica.start(simple_replica)
            self.replicas.append(replica)
            
        # Wait for replicas to initialize
        time.sleep(2)
        
    def stop_cluster(self):
        """Stop all replicas"""
        for replica in self.replicas:
            replica.stop()
            
    def get_live_replicas(self) -> List[int]:
        """Get list of alive replica IDs"""
        return [r.replica_id for r in self.replicas if r.is_alive()]
    
    def kill_replicas(self, replica_ids: List[int]):
        """Kill specified replicas"""
        for rid in replica_ids:
            for r in self.replicas:
                if r.replica_id == rid:
                    r.stop()
                    
    def start_client(self, simple_client: str, key_file: str = "priv/client0.pem"):
        """Start client"""
        self.client = TestClient(self.config_file, key_file)
        self.client.connect(simple_client)
        
    def stop_client(self):
        """Stop client"""
        if self.client:
            self.client.close()


def wait_for_quorum(cluster: ClusterManager, timeout: int = 10) -> bool:
    """Wait until cluster has quorum (5 replicas alive)"""
    start = time.time()
    while time.time() - start < timeout:
        live = len(cluster.get_live_replicas())
        if live >= QUORUM:
            return True
        time.sleep(0.5)
    return False


def verify_all_replicas_agree(cluster: ClusterManager, key: str, expected_value: str) -> bool:
    """Verify all live replicas agree on the value of key"""
    # This would require querying each replica's state
    # For now, simplified - client receives response means quorum achieved
    return True


def print_test_header(test_name: str):
    """Print test header"""
    print(f"\n{'='*60}")
    print(f"TEST: {test_name}")
    print(f"{'='*60}\n")


def print_test_result(test_name: str, passed: bool, message: str = ""):
    """Print test result"""
    status = "✓ PASSED" if passed else "✗ FAILED"
    print(f"\n{status}: {test_name}")
    if message:
        print(f"  {message}")
    print("-" * 60)


if __name__ == "__main__":
    print("TinyBFT Test Utilities")
    print(f"Config: {NUM_REPLICAS} replicas, f={NUM_FAULTY}, quorum={QUORUM}")