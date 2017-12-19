#!/bin/bash

n_points=(20 50 100 1024 2000 10000 100000) # 500000 1000000)
n_steps=(1 10 100 1000 10000) # 100000 1000000)

for p in "${n_points[@]}";
do
   for s in "${n_steps[@]}";
   do
      printf 'Checking %7i %7i  ... exec time: ' $p $s
      /usr/bin/time -f '%E' ./cuda_wave $p $s > "cuda.log"
      diff "tests/seq_${p}_${s}.log" cuda.log &> /dev/null
      ret=$?
      if [ "${ret}" != "0" ]; then
         echo "Failed. The log file is stored in cuda.log."
         exit ${ret}
      fi
      rm -f cuda.log
   done
   echo "Passed."
done
