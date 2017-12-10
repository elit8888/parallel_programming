#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 prime"
    echo "Or"
    echo "$0 int"
    exit 0
fi

# run prime
if [ "$1" == "prime" ]; then
    for i in 11 20 40 60 80 100 200 400 600 800 1000 5000 10000 100000 10000000 123456789 4294967296 9999999999 1099511627776
    do
        mpiexec -n 4 -host host,slave1,slave2,slave3 ./prime $i >> rslt_prime_mpi.txt
    done
else
    # run integration
    for i in 1 2 3 4 5 6 7 8 9 10 20 50 100 200 500 1000 10000 100000 1000000
    do
        mpiexec -n 4 -host host,slave1,slave2,slave3 ./integrate_seq $i >> rslt_int_mpi.txt
    done
fi
