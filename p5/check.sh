#!/bin/bash

input_dir="/home/data/OpenCL"
ref_dir="${input_dir}/ReferenceOutput"
test_file="./cl_hist"
# The program will generate the output file
out_file="0556519.out"

for testfile in ${input_dir}/input-*; do
    filename=$(basename "$testfile")
    ref_file=${filename/input/output}

    # Link the input file
    ln -fs ${testfile} input

    printf "Input file ${filename}, exec time: "
    /usr/bin/time -f '%E' ${test_file}
    diff "${ref_dir}/${ref_file}" ${out_file}
    ret=$?
    rm -f input

    if [ "${ret}" != "0" ]; then
        echo "Failed."
        exit ${ret}
    fi
done

echo "Passed."
