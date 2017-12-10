# MPI practice

## 1. Find the prime number

For normal version, use:
``` bash
gcc -lm -o prime_seq prime_seq.c
./prime_seq
```

For MPI version, use:
``` bash
# make sure mpi is installed.
mpicc -lm -o prime prime.c
mpiexec -n 4 -host host,slave1,slave2,slave3 ./prime <number>
```

## 2. TODO
