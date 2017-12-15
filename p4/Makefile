.PHONY: seq
seq: seq_wave

seq_wave: wave.c
	gcc -o $@ $< -lm

.PHONY: cuda
cuda: cuda_wave

cuda_wave: wave.cu
	nvcc -o $@ $<

.PHONY: all
all: seq cuda


.PHONY: check
check:
	./check.sh

.PHONY: clean
clean:
	rm -f seq_wave cuda_wave *.log
