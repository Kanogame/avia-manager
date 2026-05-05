#!/bin/bash

# Example script to start the planes dispatcher system with multiple servers

# Configuration
SERVER_BIN="./server/out"
DISPATCHER_BIN="./client-2/out"
NUM_PLANES=3
REGISTRY_FILE="/tmp/planes_registry"

# Clean up old registry
rm -f "$REGISTRY_FILE"

# Start multiple plane servers with specific coordinates
# Format: -id <plane_id> -x <x_coord> -y <y_coord>

echo "Starting plane servers..."

# Plane 1: Top-left area
$SERVER_BIN -id 101 -x -30 -y 30 &
SERVER_PIDS[0]=$!
echo "Started plane 101 at (-30, 30)"

# Plane 2: Center area
$SERVER_BIN -id 102 -x 0 -y 0 &
SERVER_PIDS[1]=$!
echo "Started plane 102 at (0, 0)"

# Plane 3: Bottom-right area
$SERVER_BIN -id 103 -x 25 -y -25 &
SERVER_PIDS[2]=$!
echo "Started plane 103 at (25, -25)"

# Wait for servers to register
sleep 1

echo "Starting dispatcher..."
$DISPATCHER_BIN &
DISPATCHER_PID=$!

echo "System running. Press Ctrl+C to stop."
echo ""
echo "Active processes:"
echo "  Dispatcher PID: $DISPATCHER_PID"
echo "  Server PIDs: ${SERVER_PIDS[@]}"

# Wait for dispatcher
wait $DISPATCHER_PID

# Clean up servers
echo "Shutting down servers..."
for pid in "${SERVER_PIDS[@]}"; do
    kill $pid 2>/dev/null
done

# Clean up registry
rm -f "$REGISTRY_FILE"

echo "Done."
