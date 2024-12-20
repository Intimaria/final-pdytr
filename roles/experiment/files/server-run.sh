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

# Run experiments
#for exp in 3 4 5 6 7; do
#    size=$((10**exp))
#    echo "Running server with buffer size: $size"
#
    # Execute server binary
#    ./server "$PORT" "$size" "$CPU_LOAD" > "server_${size}.log" 2>&1 &
#    SERVER_PID=$!

    # Wait for the server to complete
#    wait $SERVER_PID
#    if [ $? -ne 0 ]; then
#        echo "Error: Server failed for buffer size $size" >&2
#        cat "server_${size}.log"
#        exit 1
#    fi

#    echo -e "$size\tCompleted"
#    sleep 1
#done


# Execute server binary
./server "$PORT" "$BUFFER_SIZE" "$CPU_LOAD"
#> "server_${BUFFER_SIZE}.log" 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Server failed for buffer size $BUFFER_SIZE" >&2
    exit 1
fi

echo "Server experiments completed."

