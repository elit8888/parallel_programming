#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

// CL/cl.hpp is not in the path

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define DATA_SIZE (0x100000)
#define error_exit(msg) do { std::cerr << msg << std::endl; exit(EXIT_FAILURE); } while(0)

class clCompute{
  public:
    clCompute()
    {
        preparePlatformIDs();
        prepareDeviceIDs();
        prepareContext();
        // skip clGetDeviceInfo
        prepareCommandQueue();
    }
    ~clCompute()
    {
        //clReleaseCommandQueue(_command_queue);
        //clReleaseContext(_context);
    }

    void preparePlatformIDs()
    {
        cl_int ret = clGetPlatformIDs(0, 0, &_n_platforms);
        if (ret != CL_SUCCESS)  error_exit("Unable to get platforms.");
        std::cout << "Num of platforms: " << _n_platforms << std::endl;

        // get platform
        _platforms.reserve(_n_platforms);
        ret = clGetPlatformIDs(_n_platforms, &_platforms[0], &_n_platforms);
        if (ret != CL_SUCCESS)  error_exit("Unable to get platform ID.");

        /* or just
         * clGetPlatformIDs(1, &_platform, NULL);
         */
    }

    cl_platform_id getPlatformID(int ix=0)
    { return _platforms[ix]; }  // return _platform;

    void prepareDeviceIDs()
    {
        cl_uint ret_num_devices;
        cl_int ret = clGetDeviceIDs(_platforms[0], CL_DEVICE_TYPE_DEFAULT, 1, &_device_id, &ret_num_devices);
                                                // ^ can be CL_DEVICE_TYPE_GPU
        std::cout << "ret_num_devices: " << ret_num_devices << std::endl;
    }

    cl_device_id getDeviceID()
    { return _device_id; }

    void prepareContext()
    {
        //extern CL_API_ENTRY cl_context CL_API_CALL
        //clCreateContext(const cl_context_properties * /* properties */,
        //                cl_uint                 /* num_devices */,
        //                const cl_device_id *    /* devices */,
        //                void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
        //                void *                  /* user_data */,
        //                cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;
        cl_int ret;
        _context = clCreateContext(NULL, 1, &_device_id, NULL, NULL, &ret);
                                // ^ Use default chosen by the vendor
        if (_context == 0)  std::cerr << "Can't create OpenCL contxt.\n";
    }

    cl_context getContext()
    { return _context; }

    void prepareCommandQueue()
    {
        cl_int ret;
        _command_queue = clCreateCommandQueueWithProperties(_context, _device_id, 0, &ret);
                         // clCreateCommandQueue is deprecated
        if (_command_queue == 0)
        {
            clReleaseContext(_context);
            error_exit("Can't create command queue.");
        }
    }

    cl_command_queue getCommandQueue()
    { return _command_queue; }

  private:
    cl_uint _n_platforms;
    cl_device_id _device_id;
    cl_context _context;
    cl_command_queue _command_queue;
    std::vector<cl_platform_id> _platforms;  // cl_platform_id _platform;
    std::string _dev_name;
};

cl_program load_program(cl_context, cl_device_id, const char *);


int main(int argc, char *argv[])
{
    cl_int err;
    std::string dev_name;

    clCompute clc;
    cl_device_id device_id = clc.getDeviceID();
    cl_platform_id platform = clc.getPlatformID();
    cl_context context = clc.getContext();
    cl_command_queue queue = clc.getCommandQueue();

    // Get input data
    unsigned int input_size;
    unsigned int *image;
    unsigned int *histogram_results = new unsigned int[256*3];
    unsigned int a;

    std::cout << "Reading input..." << std::flush;
    std::fstream inFile("input", std::ios_base::in);
    inFile >> input_size;
    image = new unsigned int[input_size];
    for (unsigned int i = 0; i < input_size; ++i)
    {
    //while( inFile >> a ) {
        //image[i++] = a;
        inFile >> image[i];
    }
    inFile.close();
    std::cout << "Done, input size: " << input_size << std::endl;
    std::cout << "Input: ";
    for (int i = 0; i < input_size; ++i)
        std::cout << image[i] << " ";
    std::cout << std::endl;

    //cl_mem in_img = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &a[0], NULL);
    cl_mem in_img = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_uint)*input_size, NULL, NULL);
    cl_mem out_hist = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint)*256*3, NULL, NULL);
    if (in_img == 0 || out_hist == 0)  // clCreateBuffer failed
    {
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't create OpenCL buffer.");
    }

    err = clEnqueueWriteBuffer(queue, in_img, CL_TRUE, 0, sizeof(cl_uint)*input_size, (void*)image, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("[E] clEnqueueWriteBuffer: Failed to write buffer.");
    }

    // call program
    cl_program program = load_program(context, device_id, "histogram.cl");
    if (program == 0)
    {
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't load/build program");
    }

    // load kernel
    cl_kernel k_hist = clCreateKernel(program, "histogram", 0);
    if (k_hist == 0)
    {
        clReleaseProgram(program);
        clReleaseMemObject(in_img);
        clReleaseMemObject(out_hist);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        error_exit("Can't load kernel");
    }
    std::cout << "Kernel created.\n";

    // use kernel

    // set params
    clSetKernelArg(k_hist, 0, sizeof(cl_mem), &in_img);
    clSetKernelArg(k_hist, 1, sizeof(cl_mem), &out_hist);

    size_t work_size = input_size/3;
    err = clEnqueueNDRangeKernel(queue, k_hist, 1, 0, &work_size, 0, 0, 0, 0);
                                             // ^ dim of work item

    if (err == CL_SUCCESS)
    {
        std::cout << "Getting result...";
        // Get result
        err = clEnqueueReadBuffer(queue, out_hist, CL_TRUE, 0, sizeof(cl_uint)*256*3, histogram_results, 0, 0, 0);
        std::cout << "Done.\n";
    }
    if (err != CL_SUCCESS)
    {
        std::cerr << "EnqueueNDRangeKernel error.\n";
    }
    else
    {
        std::cout << "Success.\n";
        std::ofstream outFile("0556519.out", std::ios_base::out);
        for(unsigned int i = 0; i < 256 * 3; ++i) {
            if (i % 256 == 0 && i != 0)
            {
                outFile << std::endl;
            }
            outFile << histogram_results[i]<< ' ';
        }
        outFile.close();
    }

    delete [] histogram_results;
    delete [] image;
    clReleaseKernel(k_hist);
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
