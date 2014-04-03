#ifndef CLI_H
#define CLI_H

#include <cstdio>
#include <cstdlib>
#include <vector>

// OpenCL includes
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#elif __linux
#include <CL/cl.h>
#elif _WIN32
#include <CL/opencl.h>
#else
#error Platform not supported
#endif

typedef struct cli_s
{
public:
    //initialize our data we're keeping track of
    // used for  1: Discover and initialize the platforms
    cl_uint numPlatforms;

    cl_platform_id *platforms;

    // used for 2: Discover and initialize the devices
    cl_uint numDevices;
    cl_device_id *devices;

    // used for 3: Create a context
    cl_context context;

    // used for 4: Create a command queue
    cl_command_queue cmdQueue;

    // used for 7: Create and compile the program     
    cl_program program;
   
    // used for 8: Create the kernel
    cl_kernel kernel;

    // internal status to check the output of each API call
    //  this isn't working for some reason
    //std::vector<cl_int> status;

    //I think the vectors need to be constructed, even in a struct
    //std::vector<cl_mem> clMemDes;

} CLI;


extern void cliInitialize(CLI*cli, std::vector<cl_int> &errors);

extern void cliBuild(
    CLI* cli, 
    const char* programSource, 
    const char * kernel_name,
    std::vector<cl_int> &errors);

extern void cliRelease(CLI* cli);

extern void cliStatus(const cl_int err, char*stat);

extern void PrintCLIStatus(std::vector<cl_int> &errors);

extern cl_mem cliKernelArgs(
    void* ptr,              // I want to restrict this
    size_t bufferBytes,  
    int argn, 
    cl_mem_flags memflag,
    CLI* cli,
    std::vector<cl_int> &errors);

#endif