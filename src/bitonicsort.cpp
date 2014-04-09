#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <time.h>

#include "cli.h"
#include "kernels.h"
#include "stl.h"

#define CL_ERRORS 1
#define NORMALS
//#define DATA_TYPE int 
//#define DATA_SIZE 1024
#define WORK_GROUP_SIZE 64 // logical errors occur after work group size > 128

#ifndef _WIN32
#ifndef __APPLE__
#define TIME 1
#define BENCHSIZE 16
#endif
#endif


int main() 
{
    //  --------------------------
    //
    // STL stuff
    //
    //  --------------------------
	const char* stlFile = "Ring.stl";

    std::vector<cl_int> errors;
    std::vector<float> verticies;
    std::vector<float> normals;

    //std::vector<float> overt;

    if(stlRead(stlFile, verticies, normals))
    {
        std::cout<<"ERROR: reading file"<<std::endl;
        return 1;
    }

    //check sanity for verticies and normals
    if( fmod(verticies.size(),9.0) !=  0 || fmod(normals.size(),3.0) != 0 )
    {
        std::cout<<"ERROR: verticies and normals don't make sense up"<<std::endl;
        return 1;
    }

    //for (int i = 0; i < overt.size(); ++i)
    //{
    //    verticies.push_back(overt[i]);
    //}

    //  --------------------------
    //
    // pad our verticies with -1's
    //
    //  --------------------------
    unsigned int n = verticies.size()/9 - 1;
    unsigned int p2 = 0;
    
    size_t original_vertex_size = verticies.size();
    do ++p2; while( (n >>= 0x1) != 0);
    size_t padded_size = 0x1 << p2;

    unsigned int padd = 0;

    // it just needs to be larger really
    while(verticies.size()/9 < padded_size)
    {
        verticies.push_back(-1.0);
        ++padd;
    }

    //  --------------------------
    //
    // OpenCL stuff
    //
    //  --------------------------
    cl_int clStatus;

    CLI *cli_bsort = (CLI*) malloc( sizeof(CLI));
    cliInitialize(cli_bsort, errors);
    cliBuild(
        cli_bsort,
        bitonic_STL_sort_source,
        "_kbitonic_stl_sort",
        errors);
    
    // Basic initialization and declaration...
    // Execute the OpenCL kernel on the list
    // Each work item shall compare two elements.
    size_t global_size = padded_size/2;
    // This is the size of the work group.
    size_t local_size = WORK_GROUP_SIZE;
     // Calculate the Number of work groups.
    size_t num_of_work_groups = global_size/local_size;

    //Create memory buffers on the device for each vector
    cl_mem pInputBuffer_clmem = clCreateBuffer(
        cli_bsort->context, 
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
        padded_size * verticies.size()*sizeof(float), 
        (float*) &verticies.front(), 
        &clStatus);
  	errors.push_back(clStatus); 
    // create kernel
    
    clSetKernelArg(
        cli_bsort->kernel, 
        0, 
        sizeof(cl_mem), 
        (void *) &pInputBuffer_clmem);

    unsigned int stage, passOfStage, numStages, temp;
    stage = passOfStage = numStages = 0;
    
    for(temp = padded_size; temp > 1; temp >>= 1)
        ++numStages;
 
    global_size = padded_size>>1;
    local_size = WORK_GROUP_SIZE;
    
		for(stage = 0; stage < numStages; ++stage)
    {
        // stage of the algorithm
        clSetKernelArg(
            cli_bsort->kernel, 
            1, 
            sizeof(int), 
            (void *)&stage);

        // Every stage has stage + 1 passes
        for(passOfStage = 0; passOfStage < stage + 1; 
            ++passOfStage) 
        {
            // pass of the current stage
            printf("Pass no: %d\n",passOfStage);
            clStatus = clSetKernelArg(
                cli_bsort->kernel, 
                2, 
                sizeof(int), 
                (void *)&passOfStage);
   				
				errors.push_back(clStatus);
            //
            // Enqueue a kernel run call.
            // Each thread writes a sorted pair.
            // So, the number of threads (global) should be half the 
            // length of the input buffer.
            //
            clEnqueueNDRangeKernel(
                cli_bsort->cmdQueue, 
                cli_bsort->kernel, 
                1, 
                NULL,
                &global_size, 
                &local_size, 
                0, 
                NULL, 
                NULL);  

            clFinish(cli_bsort->cmdQueue);
        } //end of for passStage = 0:stage-1
    } //end of for stage = 0:numStage-1
 
    float *mapped_input_buffer =
        (float *)clEnqueueMapBuffer(
            cli_bsort->cmdQueue, 
            pInputBuffer_clmem, 
            true, 
            CL_MAP_READ, 
            0, 
            sizeof(float) *9* padded_size, 
            0, 
            NULL, 
            NULL, 
            &clStatus);

		errors.push_back(clStatus);


    //  --------------------------
    //
    // Done
    //
    //  --------------------------

    PrintCLIStatus(errors);
    std::vector<float> output;

    int count=0; 
    for (int i = 0; i < verticies.size(); ++i)
    {
        if(verticies[i] == -1.0)
            count++;
    }

    //Display the Sorted data on the screenm
    for(int i = 0; i < padded_size; i++)
    {
	  printf("i=%d: %f \n", i, mapped_input_buffer[i*9+2]); 
    }
		
    // cleanup...
    return 0;
}
