#!/bin/bash

printf 'Checking %7i %7i  ... exec time: ' $p $s
/usr/bin/time -f '%E' ./cuda_wave $p $s > "cuda.log"
diff "tests/seq_${p}_${s}.log" cuda.log &> /dev/null
ret=$?
if [ "${ret}" != "0" ]; then
 echo "Failed. The log file is stored in cuda.log."
 exit ${ret}
fi
rm -f cuda.log
echo "Passed."
