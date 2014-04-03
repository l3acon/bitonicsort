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
#define DATA_TYPE int 
#define DATA_SIZE 1024
#define WORK_GROUP_SIZE 64 // logical errors occur after work group size > 128

#ifndef _WIN32
#ifndef __APPLE__
#define TIME 1
#define BENCHSIZE 16
#endif
#endif


int main() 
{
		const char* stlFile = "Ring.stl";

    std::vector<cl_int> errors;
    std::vector<float> verticies;
    std::vector<float> normals;
        
		//later we can just use the memory in a std::vector?
    float * vertexBuffer;
    float * normalBuffer;

    //file stuff
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

    cl_int clStatus;

    CLI *cli_bsort = (CLI*) malloc( sizeof(CLI));
    cliInitialize(cli_bsort, errors);
    cliBuild(
    	cli_bsort,
			bitonic_sort_kernel_source,
			"_kbitonic_sort_kernel",
    	errors);
		
    // Basic initialization and declaration...
    // Execute the OpenCL kernel on the list
    // Each work item shall compare two elements.
    size_t global_size = DATA_SIZE/2;
    // This is the size of the work group.
    size_t local_size = WORK_GROUP_SIZE;
     // Calculate the Number of work groups.
    size_t num_of_work_groups = global_size/local_size;
    //Allocate memory and initialize the input buffer.
    DATA_TYPE *pInputBuffer = (DATA_TYPE*)malloc(
        sizeof(DATA_TYPE)*DATA_SIZE);

    for(int i =0; i< DATA_SIZE; i++)
    {
        pInputBuffer[i] =  ( (int) rand()  ) ;
        //printf("%d: %d\n", i, pInputBuffer[i]);
    }
 
    //Create memory buffers on the device for each vector
    cl_mem pInputBuffer_clmem = clCreateBuffer(
        cli_bsort->context, 
        CL_MEM_READ_WRITE |
        CL_MEM_USE_HOST_PTR,
        DATA_SIZE * sizeof(DATA_TYPE), 
        pInputBuffer, 
        &clStatus);
  	errors.push_back(clStatus); 
    // create kernel
    
    clSetKernelArg(
        cli_bsort->kernel, 
        0, 
        sizeof(cl_mem), 
        (void *)&pInputBuffer_clmem);

    unsigned int stage, passOfStage, numStages, temp;
    stage = passOfStage = numStages = 0;
    
    for(temp = DATA_SIZE; temp > 1; temp >>= 1)
        ++numStages;
 
    global_size = DATA_SIZE>>1;
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
            //printf("Pass no: %d\n",passOfStage);
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
 
    DATA_TYPE *mapped_input_buffer =  
        (DATA_TYPE *)clEnqueueMapBuffer(
            cli_bsort->cmdQueue, 
            pInputBuffer_clmem, 
            true, 
            CL_MAP_READ, 
            0, 
            sizeof(DATA_TYPE) * DATA_SIZE, 
            0, 
            NULL, 
            NULL, 
            &clStatus);

		errors.push_back(clStatus);

    //Display the Sorted data on the screen
    for(int i = 0; i < DATA_SIZE-1; i++)
    {
        if(mapped_input_buffer[i+1] < mapped_input_buffer[i])
          printf( "%d: FAILED: %d < %d \n", i, mapped_input_buffer[i+1], mapped_input_buffer[i+1]);
        //printf("%d: %d\n", i, mapped_input_buffer[i]);
    }
		
		PrintCLIStatus(errors);
    // cleanup...
    return 0;
}
