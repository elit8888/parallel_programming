#!/bin/bash

# Simple test script
# Use `make debug` to calculate with verbose output.
# Elit, 2017/11/2

iter=10000000

for cpus in 1 2 4 8 10 16 20  32 64 100 128 200 256 500 512 1000; do
    ./pi $cpus $iter
    echo ""
done
