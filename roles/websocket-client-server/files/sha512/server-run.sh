#!/bin/bash

gcc -w server.c checksum.c -lm -o server -lssl -lcrypto

echo -e "10³\t\t10⁴\t\t10⁵\t\t10⁶\t\t10⁷"
for i in {1..100}; do
for exp in 3 4 5 6 7; do
    size=$((10**exp))

    ./server 4003 $size
done
echo ""
done
