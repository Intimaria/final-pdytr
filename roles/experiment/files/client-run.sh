#!/bin/bash

# Ensure variables are set
SERVER_IP="${SERVER_IP:?Server IP is not set}"
PORT="${PORT:?Port is not set}"
CLOCK_OFFSET="$(chronyc tracking | awk '/Last offset/ {print $4}')"
CPU_LOAD="${CPU_LOAD:?CPU load is not set}"

echo "Starting client: SERVER_IP=$SERVER_IP, PORT=$PORT, CLOCK_OFFSET=$CLOCK_OFFSET, CPU_LOAD=$CPU_LOAD"


# Output headers
# echo -e "Buffer Size\tOne-Way Delay"

# Run experiments
#for exp in 3 4 5 6 7; do
#    size=$((10**exp))
#    echo "Running experiment with buffer size: $size"

    # Execute client binary and capture output
#    output=$(./client "$SERVER_IP" "$PORT" "$size" "$CLOCK_OFFSET" 2> "client_${size}.log")
#    if [ $? -ne 0 ]; then
#        echo "Error: Client failed for buffer size $size" >&2
#        cat "client_${size}.log"
#        exit 1
#    fi

    # Output results
#    echo -e "$size\t$output"
#    sleep 1
# done


# Execute client binary
output=$(./client "$SERVER_IP" "$PORT" "$BUFFER_SIZE" "$CLOCK_OFFSET" "$CPU_LOAD" 2>&1)
if [ $? -ne 0 ]; then
    echo "Error: Client failed for buffer size $BUFFER_SIZE" >&2
    echo "$output" >&2
    exit 1
fi

# Output result
echo "$output"
echo "Clock offset: $CLOCK_OFFSET"
echo "Client experiments completed."

