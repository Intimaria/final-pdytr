#!/bin/bash

gcc -w server.c checksum.c -o server -lssl -lcrypto

PORT=4003
CPU_LOAD=1

echo -e "Buffer Size\tServer Status"
for i in {1..100}; do
    for exp in 3 4 5 6 7; do
        size=$((10**exp))
        ./server $PORT $size $CPU_LOAD &
        SERVER_PID=$!
        wait $SERVER_PID
        echo -e "$size\tCompleted"
    done
done
