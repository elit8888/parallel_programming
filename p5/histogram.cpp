#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define error_exit(msg) do { std::cerr << msg << std::endl; exit(EXIT_FAILURE); } while(0)

cl_program load_program(cl_context, cl_device_id, const char *);


int main(int argc, char *argv[])
{
    cl_int err;
    std::string dev_name;

    // Select a platform
    cl_uint num;
    err = clGetPlatformIDs(0, 0, &num);
    if (err != CL_SUCCESS)  error_exit("Unable to get platforms.");
    std::cout << "Platform ID: " << num << std::endl;

    // Get platform id
    std::vector<cl_platform_id> platforms(num);
    err = clGetPlatformIDs(num, &platforms[0], &num);
    if (err != CL_SUCCESS)  error_exit("Unable to get platform ID.");

    // set context
    cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms[0]), 0 };
    cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, NULL);
    if (context == 0)   error_exit("Can't create OpenCL context.");

    // Get context info
    size_t cb;
    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    std::vector<cl_device_id> devices(cb / sizeof(cl_device_id));
    clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, &devices[0], 0);

    // Get device info
    clGetDeviceInfo(devices[0], CL_DEVICE_NAME, 0, NULL, &cb);
    dev_name.resize(cb);
    clGetDeviceInfo(devices[0], CL_DEVICE_NAME, cb, &dev_name[0], 0);
    std::cout << "Device: " << dev_name << std::endl;

    // Build command queue
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, devices[0], 0, 0);
    if (queue == 0)
    {
        clReleaseContext(context);
        error_exit("Can't create command queue.");
    }

    // main process
    const int DATA_SIZE = 1048576;
    std::vector<float> a(DATA_SIZE), b(DATA_SIZE), res(DATA_SIZE);
    for (int i = 0; i < DATA_SIZE; i++)
    {
        a[i] = std::rand();
        b[i] = std::rand();
    }

    cl_mem cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &a[0], NULL);
    cl_mem cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &b[0], NULL);
    cl_mem cl_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
    if (cl_a == 0 || cl_b == 0 || cl_res == 0)  // createbuffer failed
    {
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't create OpenCL buffer.");
    }

    // call program
    cl_program program = load_program(context, devices[0], "histogram.cl");
    if (program == 0)
    {
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't load/build program");
    }

    // load kernel
    cl_kernel adder = clCreateKernel(program, "adder", 0);
    if (adder == 0)
    {
        clReleaseProgram(program);
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't load kernel");
    }

    // use kernel

    // set params
    clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
    clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_b);
    clSetKernelArg(adder, 2, sizeof(cl_mem), &cl_res);

    size_t work_size = DATA_SIZE;
    err = clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, 0);
                                            // ^ dim of work item

    if (err == CL_SUCCESS)
    {
        err = clEnqueueReadBuffer(queue, cl_res, CL_TRUE, 0, sizeof(float)*DATA_SIZE, &res[0], 0, 0, 0);
                                              // ^ Only return if tesk done
    }

    if (err == CL_SUCCESS)
    {
        bool correct = true;
        for (int i = 0; i < DATA_SIZE; i++)
        {
            if (a[i] + b[i] != res[i])
            {
                correct = false;
                break;
            }
        }

        if (correct)    std::cout << "Correct\n";
        else    std::cout << "Incorrect\n";
    }
    else
        std::cerr << "Can't run kernel or read back data." << std::endl;

    clReleaseKernel(adder);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}


cl_program load_program(cl_context context, cl_device_id device_id, const char *fname)
{
    std::ifstream in(fname, std::ios_base::binary);
    if (!in.good()) error_exit("load_program load file failed.");

    // get file size
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source
    std::vector<char> data(length + 1);
    in.read(&data[0], length);
    data[length] = 0;

    // create and build program
    const char *source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    if (program == 0)
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
