CC = g++
C_OPTION =

.PHONY: all
all: seq opencl

.PHONY: seq
seq: seq_hist

seq_hist: image-histogram_seq.cpp
	$(CC) $(C_OPTION) -o $@ $<

.PHONY: opencl
opencl: cl_hist

cl_hist: histogram.cpp histogram.cl
	$(CC) $(C_OPTION) -o $@ $< -lOpenCL

.PHONY: check
check:
	./check.sh

.PHONY: rm_data
rm_data:
	rm -f *.log *.out

.PHONY: clean
clean:
	rm -f seq_hist cl_hist *.log *.out
