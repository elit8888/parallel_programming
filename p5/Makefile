.PHONY: all
all: seq opencl opencl_oop

.PHONY: seq
seq: seq_hist

seq_hist: image-histogram_seq.cpp
	g++ -o $@ $<

.PHONY: opencl
opencl: cl_hist

cl_hist: histogram.cpp histogram.cl
	g++ -o $@ $< -lOpenCL

.PHONY: opencl_oop
opencl_oop: cl_hist_oop

cl_hist_oop: histogram_oop.cpp histogram.cl
	g++ -o $@ $< -lOpenCL


.PHONY: check
check:
	./check.sh

.PHONY: clean
clean:
	rm -f seq_hist cl_hist cl_hist_oop *.log
