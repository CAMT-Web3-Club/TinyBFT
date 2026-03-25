#!/bin/bash
# Generate RSA keys for 7 replicas + 1 client
# Using faster key generation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KEYS_DIR="$SCRIPT_DIR/priv"

mkdir -p "$KEYS_DIR"

echo "Generating RSA keys..."

# Generate replica keys (r0-r6) with smaller key size for faster generation
for i in $(seq 0 6); do
    echo "Generating r${i}..."
    openssl genrsa -out "$KEYS_DIR/r${i}_priv.pem" 1024 2>/dev/null &
done

# Generate client key
openssl genrsa -out "$KEYS_DIR/client0_priv.pem" 1024 2>/dev/null &

# Wait for all background processes
wait

# Extract public keys
for i in $(seq 0 6); do
    if [ -f "$KEYS_DIR/r${i}_priv.pem" ]; then
        openssl rsa -in "$KEYS_DIR/r${i}_priv.pem" -pubout -out "$KEYS_DIR/r${i}.pub" 2>/dev/null
        echo "r${i} done"
    fi
done

if [ -f "$KEYS_DIR/client0_priv.pem" ]; then
    openssl rsa -in "$KEYS_DIR/client0_priv.pem" -pubout -out "$KEYS_DIR/client0.pub" 2>/dev/null
    echo "client0 done"
fi

echo "Keys generated!"
ls -la "$KEYS_DIR/"