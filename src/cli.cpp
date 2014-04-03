
// opencl interface
// the wrappers make writing code in 
// OpenCL cleaner and make it easier
// to keep track of OpenCL objects

#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "cli.h"

#define STATUS_CHAR_SIZE 35

using namespace std;

// Initialize our CLI wrapper
// get OpenCL platform IDs
// get OpenCL device IDs
// create OpenCL Context and Command Queue
void cliInitialize(CLI*cli, std::vector<cl_int> &errors)
{
    //-----------------------------------------------------
    // STEP 1: Discover and initialize the platforms
    //-----------------------------------------------------
    // Use clGetPlatformIDs() to retrieve the number of 
    // platforms
    cl_int localstatus;
    localstatus = clGetPlatformIDs(0, NULL, &cli->numPlatforms);
    errors.push_back(localstatus);

    // Allocate enough space for each platform
    cli->platforms = (cl_platform_id*)malloc(cli->numPlatforms * sizeof(cl_platform_id));

    // Fill in platforms with clGetPlatformIDs()
    localstatus = clGetPlatformIDs(cli->numPlatforms, cli->platforms, 
                NULL);

    errors.push_back(localstatus);

    //-----------------------------------------------------
    // STEP 2: Discover and initialize the devices
    //----------------------------------------------------- 
    // Use clGetDeviceIDs() to retrieve the number of 
    // devices present
    localstatus = clGetDeviceIDs(
        cli->platforms[0], 
        CL_DEVICE_TYPE_ALL, 
        0,
        NULL, 
        &cli->numDevices);
    errors.push_back(localstatus);


    // Allocate enough space for each device
    cli->devices = 
        (cl_device_id*)malloc(
            cli->numDevices * sizeof(cl_device_id));

    // Fill in devices with clGetDeviceIDs()
    localstatus = clGetDeviceIDs(
        cli->platforms[0], 
        CL_DEVICE_TYPE_ALL,
        cli->numDevices, 
        cli->devices, 
        NULL);
    errors.push_back(localstatus);

    //-----------------------------------------------------
    // STEP 3: Create a context
    //----------------------------------------------------- 
    
		// Create a context using clCreateContext() and 
    // associate it with the devices
    cli->context = clCreateContext(
        NULL, 
        cli->numDevices, 
        cli->devices, 
        NULL, 
        NULL, 
        &localstatus);
    errors.push_back(localstatus);

    //-----------------------------------------------------
    // STEP 4: Create a command queue
    //----------------------------------------------------- 
    // Create a command queue using clCreateCommandQueue(),
    // and associate it with the device you want to execute 
    // on
    cli->cmdQueue = clCreateCommandQueue(
        cli->context, 
        cli->devices[0], 
        0, 
        &localstatus);
    errors.push_back(localstatus);

    return ;
}

// wraper function for kernel arguments
// this reduces the code required to
// setup buffers and set arguments
cl_mem cliKernelArgs(
    void* ptr,              // I want to restrict this
    size_t bufferBytes,  
    int argn, 
    cl_mem_flags memflag,
    CLI* cli,
    std::vector<cl_int> &errors)
{
    cl_int localstatus;
    cl_mem clmemDes;
    clmemDes = clCreateBuffer(
        cli->context,
        memflag,
        bufferBytes,
        NULL,
        &localstatus);

    if(memflag == CL_MEM_READ_ONLY)
        localstatus = clEnqueueWriteBuffer(
            cli->cmdQueue,
            clmemDes,
            CL_FALSE,
            0,
            bufferBytes,
            ptr,
            0,
            NULL,
            NULL);
    localstatus = clSetKernelArg(
        cli->kernel,
        argn,
        sizeof(cl_mem),
        &clmemDes);

    errors.push_back(localstatus);

    return clmemDes;
}

// wrapper for building OpenCL program
// 
void cliBuild (
    CLI* cli, 
    const char* programSource, 
    const char * kernel_name,
    std::vector<cl_int> &errors)
{
    cl_int localstatus;

    //-----------------------------------------------------
    // STEP 7: Create and compile the program
    //----------------------------------------------------- 

    // Create a program using clCreateProgramWithSource()
    cli->program = clCreateProgramWithSource(
        cli->context, 
        1, 
        (const char**)&programSource,                                 
        NULL, 
        &localstatus);
    errors.push_back(localstatus);

    // Build (compile) the program for the devices with
    // clBuildProgram()
    localstatus = clBuildProgram(
        cli->program, 
        cli->numDevices, 
        cli->devices, 
        NULL, 
        NULL, 
        NULL);
    
    errors.push_back(localstatus);
    //-----------------------------------------------------
    // STEP 8: Create the kernel
    //----------------------------------------------------- 
    // Use clCreateKernel() to create a kernel from the 
    // vector addition function (named "vecadd")
    cli->kernel = clCreateKernel(cli->program, kernel_name , &localstatus);
    errors.push_back(localstatus);

    return;
}

// release CLI memory
void cliRelease(CLI* cli)
{
	clReleaseKernel(cli->kernel);
    clReleaseProgram(cli->program);
    clReleaseCommandQueue(cli->cmdQueue);
    clReleaseContext(cli->context);
    free(cli->platforms);
    free(cli->devices);

}

// translate OpenCL status codes to human
// readable errors
void cliStatus(const cl_int err, char* stat)
{
    //printing is inefficient anyway
    const char zero[STATUS_CHAR_SIZE] = {};
    strcpy(stat,zero);
    switch (err) 
    {
        case CL_SUCCESS:                            strcpy(stat, "Success!"); break;
        case CL_DEVICE_NOT_FOUND:                   strcpy(stat, "Device not found."); break;
        case CL_DEVICE_NOT_AVAILABLE:               strcpy(stat, "Device not available"); break;
        case CL_COMPILER_NOT_AVAILABLE:             strcpy(stat, "Compiler not available"); break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      strcpy(stat, "Memory object allocation failure"); break;
        case CL_OUT_OF_RESOURCES:                   strcpy(stat, "Out of resources"); break;
        case CL_OUT_OF_HOST_MEMORY:                 strcpy(stat, "Out of host memory"); break;
        case CL_PROFILING_INFO_NOT_AVAILABLE:       strcpy(stat, "Profiling information not available"); break;
        case CL_MEM_COPY_OVERLAP:                   strcpy(stat, "Memory copy overlap"); break;
        case CL_IMAGE_FORMAT_MISMATCH:              strcpy(stat, "Image format mismatch"); break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         strcpy(stat, "Image format not supported"); break;
        case CL_BUILD_PROGRAM_FAILURE:              strcpy(stat, "Program build failure"); break;
        case CL_MAP_FAILURE:                        strcpy(stat, "Map failure"); break;
        case CL_INVALID_VALUE:                      strcpy(stat, "Invalid value"); break;
        case CL_INVALID_DEVICE_TYPE:                strcpy(stat, "Invalid device type"); break;
        case CL_INVALID_PLATFORM:                   strcpy(stat, "Invalid platform"); break;
        case CL_INVALID_DEVICE:                     strcpy(stat, "Invalid device"); break;
        case CL_INVALID_CONTEXT:                    strcpy(stat, "Invalid context"); break;
        case CL_INVALID_QUEUE_PROPERTIES:           strcpy(stat, "Invalid queue properties"); break;
        case CL_INVALID_COMMAND_QUEUE:              strcpy(stat, "Invalid command queue"); break;
        case CL_INVALID_HOST_PTR:                   strcpy(stat, "Invalid host pointer"); break;
        case CL_INVALID_MEM_OBJECT:                 strcpy(stat, "Invalid memory object"); break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    strcpy(stat, "Invalid image format descriptor"); break;
        case CL_INVALID_IMAGE_SIZE:                 strcpy(stat, "Invalid image size"); break;
        case CL_INVALID_SAMPLER:                    strcpy(stat, "Invalid sampler"); break;
        case CL_INVALID_BINARY:                     strcpy(stat, "Invalid binary"); break;
        case CL_INVALID_BUILD_OPTIONS:              strcpy(stat, "Invalid build options"); break;
        case CL_INVALID_PROGRAM:                    strcpy(stat, "Invalid program"); break;
        case CL_INVALID_PROGRAM_EXECUTABLE:         strcpy(stat, "Invalid program executable"); break;
        case CL_INVALID_KERNEL_NAME:                strcpy(stat, "Invalid kernel name"); break;
        case CL_INVALID_KERNEL_DEFINITION:          strcpy(stat, "Invalid kernel definition"); break;
        case CL_INVALID_KERNEL:                     strcpy(stat, "Invalid kernel"); break;
        case CL_INVALID_ARG_INDEX:                  strcpy(stat, "Invalid argument index"); break;
        case CL_INVALID_ARG_VALUE:                  strcpy(stat, "Invalid argument value"); break;
        case CL_INVALID_ARG_SIZE:                   strcpy(stat, "Invalid argument size"); break;
        case CL_INVALID_KERNEL_ARGS:                strcpy(stat, "Invalid kernel arguments"); break;
        case CL_INVALID_WORK_DIMENSION:             strcpy(stat, "Invalid work dimension"); break;
        case CL_INVALID_WORK_GROUP_SIZE:            strcpy(stat, "Invalid work group size"); break;
        case CL_INVALID_WORK_ITEM_SIZE:             strcpy(stat, "Invalid work item size"); break;
        case CL_INVALID_GLOBAL_OFFSET:              strcpy(stat, "Invalid global offset"); break;
        case CL_INVALID_EVENT_WAIT_LIST:            strcpy(stat, "Invalid event wait list"); break;
        case CL_INVALID_EVENT:                      strcpy(stat, "Invalid event"); break;
        case CL_INVALID_OPERATION:                  strcpy(stat, "Invalid operation"); break;
        case CL_INVALID_GL_OBJECT:                  strcpy(stat, "Invalid OpenGL object"); break;
        case CL_INVALID_BUFFER_SIZE:                strcpy(stat, "Invalid buffer size"); break;
        case CL_INVALID_MIP_LEVEL:                  strcpy(stat, "Invalid mip-map level"); break;
    }
    return; 
}

// print all CLI status/errors
//
void PrintCLIStatus(std::vector<cl_int> &errors)
{
    char tmp[STATUS_CHAR_SIZE];

    for( std::vector<cl_int>::const_iterator iter = errors.begin(); iter != errors.end(); ++iter)
    {
        cliStatus(*iter, tmp);
        printf("%s\n", tmp);
    }
}
