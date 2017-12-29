__kernel void adder(__global const float *a, __global const float *b, __global float *result)
{
    int idx = get_global_id(0);
    result[idx] = a[idx] + b[idx];
}

__kernel void histogram(__global unsigned int *image_data, __global unsigned int *result)
{
    unsigned int col = get_global_id(0);
    if (col < 256*3)    result[col] = 0;
    mem_fence(CLK_GLOBAL_MEM_FENCE);
    col *= 3;
    for (int i = 0; i < 3; ++i)
    {
        atomic_inc(result + 256*i + image_data[col + i]);
    }
}
