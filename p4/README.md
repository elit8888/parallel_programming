# Wave Compute

This project use CUDA for parallization.

To use the code:
``` bash
make seq cuda
# It will generate seq_wave and cuda_wave.

./seq_wave <num_points> <num_steps>
./cuda_wave <num_points> <num_steps>
# num_points: 20 - 1000000
# num_steps: 1 - 1000000
```
