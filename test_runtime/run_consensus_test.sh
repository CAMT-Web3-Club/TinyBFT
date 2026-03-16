#!/nix/store/2hjsch59amjs3nbgh7ahcfzm2bfwl8zi-bash-5.3p9/bin/bash

# TinyBFT Test Script
# Starts 4 replicas and runs a test client

PROJECT_DIR="/home/phukrit7171/Development/TinyBFT"
cd "$PROJECT_DIR"

echo "=== TinyBFT Consensus Test ==="
echo ""

# Kill any existing processes on our ports
echo "Cleaning up any existing processes..."
pkill -f "simple_replica" 2>/dev/null || true
pkill -f "simple_client" 2>/dev/null || true
sleep 1

# Build if needed
echo "Building..."
cd "$PROJECT_DIR/build" && cmake .. && make -j4 > /dev/null 2>&1
cd "$PROJECT_DIR/examples/build" && cmake .. && make -j4 > /dev/null 2>&1

echo "Starting 4 replicas..."

# Start replicas in background with absolute paths
"$PROJECT_DIR/examples/build/simple_replica" \
    "$PROJECT_DIR/test_runtime/test.conf" \
    "$PROJECT_DIR/test_runtime/priv/r0.pem" \
    5679 \
    > "$PROJECT_DIR/test_runtime/replica0.log" 2>&1 &
echo "  Replica 0 started on port 5679 (PID: $!)"

"$PROJECT_DIR/examples/build/simple_replica" \
    "$PROJECT_DIR/test_runtime/test.conf" \
    "$PROJECT_DIR/test_runtime/priv/r1.pem" \
    5680 \
    > "$PROJECT_DIR/test_runtime/replica1.log" 2>&1 &
echo "  Replica 1 started on port 5680 (PID: $!)"

"$PROJECT_DIR/examples/build/simple_replica" \
    "$PROJECT_DIR/test_runtime/test.conf" \
    "$PROJECT_DIR/test_runtime/priv/r2.pem" \
    5681 \
    > "$PROJECT_DIR/test_runtime/replica2.log" 2>&1 &
echo "  Replica 2 started on port 5681 (PID: $!)"

"$PROJECT_DIR/examples/build/simple_replica" \
    "$PROJECT_DIR/test_runtime/test.conf" \
    "$PROJECT_DIR/test_runtime/priv/r3.pem" \
    5682 \
    > "$PROJECT_DIR/test_runtime/replica3.log" 2>&1 &
echo "  Replica 3 started on port 5682 (PID: $!)"

# Wait for replicas to initialize
echo "Waiting for replicas to initialize (5 seconds)..."
sleep 5

# Check if replicas are running
echo ""
echo "Replica logs:"
for i in 0 1 2 3; do
    echo "--- Replica $i ---"
    tail -5 "$PROJECT_DIR/test_runtime/replica${i}.log" 2>/dev/null || echo "(no log)"
done

echo ""
echo "Running test client..."
echo "----------------------------------------"
"$PROJECT_DIR/examples/build/test_client" 2>&1
echo "----------------------------------------"

echo ""
echo "Stopping replicas..."
pkill -f "simple_replica" 2>/dev/null || true
sleep 1

echo ""
echo "=== Test Complete ==="
