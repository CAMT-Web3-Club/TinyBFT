#!/bin/bash
# Run 7-replica cluster for integration testing on Linux

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$SCRIPT_DIR/../examples/build"
CONFIG_FILE="$SCRIPT_DIR/test_7r.conf"

REPLICA_BIN="$EXAMPLES_DIR/simple_replica"
CLIENT_BIN="$EXAMPLES_DIR/simple_client"

# Base ports
PORT_BASE=7001

# Check binaries exist
if [ ! -f "$REPLICA_BIN" ]; then
    echo "ERROR: Replica binary not found: $REPLICA_BIN"
    echo "Build examples first: cd examples/build && cmake .. && make"
    exit 1
fi

if [ ! -f "$CLIENT_BIN" ]; then
    echo "ERROR: Client binary not found: $CLIENT_BIN"
    exit 1
fi

echo "Starting 7-replica cluster..."
echo "Config: $CONFIG_FILE"
echo "Replicas: 7, Faulty: 2, Quorum: 5"

# Start replicas in background
for i in $(seq 0 6); do
    PORT=$((PORT_BASE + i))
    KEY_FILE="$SCRIPT_DIR/priv/r${i}.pem"
    echo "Starting replica $i on port $PORT..."
    $REPLICA_BIN "$CONFIG_FILE" "$KEY_FILE" "$PORT" &
done

echo ""
echo "All replicas started!"
echo "Use 'killall simple_replica' to stop all replicas"
echo ""
echo "To test manually:"
echo "  $CLIENT_BIN $CONFIG_FILE $SCRIPT_DIR/priv/client0.pem"
echo ""
echo "Test commands:"
echo "  SET key=value"
echo "  GET key"
echo "  QUIT"