#!/bin/bash

# Ensure variables are set
PORT="${PORT:?Port is not set}"
CPU_LOAD="${CPU_LOAD:?CPU load flag is not set}"

# Kill other processes running on the same port
if pgrep -f "./server $PORT" > /dev/null; then
    echo "Killing existing server processes on port $PORT"
    pkill -f "./server $PORT"
fi

echo "Starting server: PORT=$PORT, CPU_LOAD=$CPU_LOAD"

# Execute server binary
./server "$PORT" "$BUFFER_SIZE" "$CPU_LOAD"
#> "server_${BUFFER_SIZE}.log" 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Server failed for buffer size $BUFFER_SIZE" >&2
    exit 1
fi

echo "Server experiments completed."

