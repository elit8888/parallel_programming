.PHONY: seq
seq: seq_hist

seq_hist: image-histogram_seq.cpp
	g++ -o $@ $<

.PHONY: opencl
opencl: cuda_wave

cuda_wave: wave.cu
	nvcc -o $@ $<

.PHONY: all
all: seq cuda


.PHONY: check
check:
	./check.sh

.PHONY: clean
clean:
	rm -f seq_hist cuda_hist *.log
