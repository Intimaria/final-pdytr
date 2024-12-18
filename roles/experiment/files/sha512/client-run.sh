#!/bin/bash

gcc -w client.c checksum.c -o client -lssl -lcrypto

SERVER_IP="192.168.0.21"
PORT=4003
CLOCK_OFFSET=0.0

echo -e "Buffer Size\tOne-Way Delay "
for i in {1..100}; do
    for exp in 3 4 5 6 7; do
        size=$((10**exp))
        output=$(./client $SERVER_IP $PORT $size $CLOCK_OFFSET)
        echo -e "$size\t$output"
    done
done
