#!/bin/bash
# TinyBFT Test Script - Run manually to test consensus

PROJECT_DIR="/home/phukrit7171/Development/TinyBFT"

echo "=== TinyBFT Consensus Test ==="

# Clean up
pkill -9 -f simple 2>/dev/null || true
sleep 2

# Build
echo "Building..."
cd "$PROJECT_DIR/build" 
cmake .. -DDISABLE_MULTICAST=1 >/dev/null 2>&1 
make -j4 >/dev/null 2>&1 
cd "$PROJECT_DIR/examples/build"
cmake .. >/dev/null 2>&1 
make -j4 >/dev/null 2>&1 

echo "Starting 4 replicas..."

# Start replicas
$PROJECT_DIR/examples/build/simple_replica "$PROJECT_DIR/test_runtime/test.conf" "$PROJECT_DIR/test_runtime/priv/r0.pem" 5679 >/tmp/r0.log 2>&1 &
echo "  Replica 0 started (PID: $!)"

sleep 2

$PROJECT_DIR/examples/build/simple_replica "$PROJECT_DIR/test_runtime/test.conf" "$PROJECT_DIR/test_runtime/priv/r1.pem" 5680 >/tmp/r1.log 2>&1 &
echo "  Replica 1 started (PID: $!)"

sleep 2

$PROJECT_DIR/examples/build/simple_replica "$PROJECT_DIR/test_runtime/test.conf" "$PROJECT_DIR/test_runtime/priv/r2.pem" 5681 >/tmp/r2.log 2>&1 &
echo "  Replica 2 started (PID: $!)"

sleep 2

$PROJECT_DIR/examples/build/simple_replica "$PROJECT_DIR/test_runtime/test.conf" "$PROJECT_DIR/test_runtime/priv/r3.pem" 5682 >/tmp/r3.log 2>&1 &
echo "  Replica 3 started (PID: $!)"

sleep 5

echo ""
echo "Replica logs:"
echo "--- R0 ---"
cat /tmp/r0.log
echo "--- R1 ---"
cat /tmp/r1.log
echo "--- R2 ---"
cat /tmp/r2.log
echo "--- R3 ---"
cat /tmp/r3.log

echo ""
echo "Starting client..."
$PROJECT_DIR/examples/build/simple_client "$PROJECT_DIR/test_runtime/test.conf" "$PROJECT_DIR/test_runtime/priv/client0.pem"

echo ""
echo "Cleaning up..."
pkill -9 -f simple 2>/dev/null || true

echo "Done!"
