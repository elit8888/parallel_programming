#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

// CL/cl.hpp is not in the path
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define DATA_SIZE (0x100000)
#define RESULT_SIZE 768     // 256*3
#define error_exit(msg) do { std::cerr << msg << std::endl; exit(EXIT_FAILURE); } while(0)

cl_program load_program(cl_context, cl_device_id, const char *);

int main(int argc, char *argv[])
{
    cl_int err;
    cl_uint n_platforms, n_devices;
    cl_device_id device_id;
    std::vector<cl_platform_id> platforms;
    cl_context context;
    cl_command_queue command_queue;

    /* get platform ids */
    err = clGetPlatformIDs(0, NULL, &n_platforms);
    if (err != CL_SUCCESS)  error_exit("Unable to get platforms.");
#ifdef DEBUG
    std::cout << "# of platforms: " << n_platforms << std::endl;
#endif
    platforms.reserve(n_platforms);
    err = clGetPlatformIDs(n_platforms, &platforms[0], NULL);
    if (err != CL_SUCCESS)  error_exit("Unable to get platform ID.");

    /* get device ids */
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &n_devices);
    // Just get the first one
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
                                                        // ^ originally num_devices
                                                        //     ^ originally a vector::data()
#ifdef DEBUG
    std::cout << "ret_num_devices: " << n_devices << std::endl;
#endif
    /* create context */
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
                           // ^ Use default chosen by the vendor
    if (context == 0)  std::cerr << "Can't create OpenCL contxt.\n";
    /* create command queue */
    command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
                 // clCreateCommandQueue is deprecated
    if (command_queue == 0)
    {
        clReleaseContext(context);
        error_exit("Can't create command queue.");
    }

    /* Get input data */
    unsigned long int input_size, i = 0;
    unsigned int *image;
    unsigned int histogram_results[RESULT_SIZE];
    void *map_memory;
    char *begin, *end;
    struct stat sb;

#ifdef DEBUG
    std::cout << "Reading input..." << std::flush;
#endif
    int fd = open("input", O_RDONLY);
    fstat(fd, &sb); // get file size
    map_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_memory == MAP_FAILED)
    {
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        error_exit("mmap failed.");
    }
    begin = (char *)map_memory;
    //ref: https://stackoverflow.com/a/17465786/7194988
    input_size = strtoul(begin, &end, 10);
    image = new unsigned int[input_size];
    while (i < input_size)
    {
        image[i++] = strtoul(end, &end, 10);
        if (end == NULL)    break;
    }

    munmap(map_memory, sb.st_size);
    close(fd);
#ifdef DEBUG
    std::cout << "Done, input size: " << input_size << std::endl;
#endif
    if (input_size == 0 || i < input_size)
    {
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        if (input_size == 0)
            error_exit("Input size is 0.");
        else
            error_exit("Didn't get all data.");
    }
#ifdef DEBUG
    if (input_size < 0x10000)       // Don't print too large data
    {
        std::cout << "Input: ";
        for (i = 0; i < input_size; ++i)
            std::cout << image[i] << " ";
        std::cout << std::endl;
    }
#endif

    /* Allocate Device Memory */
    cl_mem in_img = clCreateBuffer(context,
                                   CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(unsigned int) * input_size,
                                   (void*)image,/*.data(),*/
                                   NULL);
    cl_mem out_hist = clCreateBuffer(context,
                                     CL_MEM_WRITE_ONLY,
                                     sizeof(unsigned int) * RESULT_SIZE,
                                     NULL,
                                     NULL);
    delete [] image;    // not used after this, delete it.
    if (in_img == 0 || out_hist == 0)  // clCreateBuffer failed
    {
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        error_exit("Can't create OpenCL buffer.");
    }

    // call program
    cl_program program = load_program(context, device_id, "histogram.cl");
    if (program == 0)
    {
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        error_exit("Can't load/build program");
    }

    // load kernel
    cl_kernel k_hist = clCreateKernel(program, "histogram", &err);
    if (k_hist == 0 || err != CL_SUCCESS)
    {
        clReleaseProgram(program);
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        error_exit("Can't load kernel");
    }
#ifdef DEBUG
    std::cout << "Kernel created.\n";
#endif

    // use kernel

    // set params
    clSetKernelArg(k_hist, 0, sizeof(cl_mem), &in_img);
    clSetKernelArg(k_hist, 1, sizeof(cl_mem), &out_hist);

    size_t work_size = input_size/3;
    err = clEnqueueNDRangeKernel(command_queue  /* command_queue */,
                                 k_hist         /* kernel */,
                                 1              /* work_dim */,
                                 NULL           /* global_work_offset */,
                                 &work_size     /* global_work_size */,
                                 NULL           /* local_work_size */,
                                 0              /* num_events_in_wait_list */,
                                 NULL           /* event_wait_list */,
                                 NULL           /* event */);

    if (err == CL_SUCCESS)
    {
#ifdef DEBUG
        std::cout << "Getting result...";
#endif
        clFinish(command_queue);
        // Get result
        err = clEnqueueReadBuffer(command_queue        /* command_queue */,
                                  out_hist     /* buffer */,
                                  CL_TRUE      /* blocking_read */,
                                  0            /* offset */,
                                  sizeof(unsigned int)*RESULT_SIZE  /* size */,
                                  histogram_results                 /* ptr */,
                                  0            /* num_events_in_wait_list */,
                                  NULL         /* event_wait_list */,
                                  NULL         /* event */);
    }
    if (err != CL_SUCCESS)
    {
        clReleaseKernel(k_hist);
        clReleaseProgram(program);
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        error_exit("EnqueueNDRangeKernel error.");
    }
    else
    {
#ifdef DEBUG
        std::cout << "Success.\n";
#endif
        std::ofstream outFile("0556519.out", std::ios_base::out);
        for(i = 0; i < RESULT_SIZE; ++i) {
            if (i % 256 == 0 && i != 0) outFile << std::endl;
            outFile << histogram_results[i] << ' ';
        }
        outFile.close();
    }

    clReleaseKernel(k_hist);
    clReleaseProgram(program);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    return 0;
}

cl_program load_program(cl_context context, cl_device_id device_id, const char *fname)
{
    cl_int err;
    std::ifstream in(fname, std::ios_base::binary);
    if (!in.good()) error_exit("load_program load file failed.");

    // get file size
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source
    std::vector<char> data(length + 1);
    in.read(data.data(), length);
    data[length] = 0;

    // create and build program
    const char *source = data.data();
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, &err);
    if (program == 0 || err != CL_SUCCESS)
    {
        std::cerr << "load_program clCreateProgramWithSource failed.\n";
        return program;
    }

    if (clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS)
    {
        // Get error message
        size_t len;
        char *buffer;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        buffer = new char[len];
        std::cerr << "load_program clBuildProgram failed.\n";
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
        std::cerr << buffer << std::endl;
        delete buffer;
        return 0;
    }

    return program;
}
