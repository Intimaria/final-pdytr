#!/bin/bash

gcc -w client.c checksum.c -lm -o client -lssl -lcrypto

echo -e "10³\t\t10⁴\t\t10⁵\t\t10⁶\t\t10⁷"
for i in {1..100}; do
for exp in 3 4 5 6 7; do
    size=$((10**exp))

    ./client 192.168.77.17 4003 $size
done
echo ""
done
