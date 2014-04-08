#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 27
#define VERTEX_SIZE 9

void bitonic_sort(                          		  
            float *input_ptr,                   	 
            const unsigned int stage,                    
            const int passOfStage, 
			int threadId)
{                                         
                                          
  unsigned int pairDistance = 1 << (stage - passOfStage);    
  unsigned int blockWidth = 2 * pairDistance;     
  unsigned int temp; 
                                                        
  int compareResult;                                     
  unsigned int  leftId = (threadId & (pairDistance - 1)) + (threadId >> (stage - passOfStage) ) * blockWidth;    
  unsigned int  rightId = leftId + pairDistance;     
       
  float leftElement[9], rightElement[9];    
  float *greater, *lesser;    
  
  leftElement[0] = input_ptr[leftId*9 + 0];   
  leftElement[1] = input_ptr[leftId*9 + 1];   
  leftElement[2] = input_ptr[leftId*9 + 2];   
  leftElement[3] = input_ptr[leftId*9 + 3];   
  leftElement[4] = input_ptr[leftId*9 + 4];   
  leftElement[5] = input_ptr[leftId*9 + 5];   
  leftElement[6] = input_ptr[leftId*9 + 6];   
  leftElement[7] = input_ptr[leftId*9 + 7];   
  leftElement[8] = input_ptr[leftId*9 + 8];   
  rightElement[0] = input_ptr[rightId*9 + 0];
  rightElement[1] = input_ptr[rightId*9 + 1];   
  rightElement[2] = input_ptr[rightId*9 + 2];   
  rightElement[3] = input_ptr[rightId*9 + 3];   
  rightElement[4] = input_ptr[rightId*9 + 4];   
  rightElement[5] = input_ptr[rightId*9 + 5];   
  rightElement[6] = input_ptr[rightId*9 + 6];   
  rightElement[7] = input_ptr[rightId*9 + 7];   
  rightElement[8] = input_ptr[rightId*9 + 8];   

  unsigned int sameDirectionBlockWidth = threadId >> stage;      
  unsigned int sameDirection = sameDirectionBlockWidth & 0x1;   
     
  temp = sameDirection ? rightId : temp;   
  rightId = sameDirection ? leftId : rightId;   
  leftId = sameDirection ? temp : leftId;   
     
  compareResult = (leftElement[2] < rightElement[2]);   
  greater = compareResult ? rightElement : leftElement;   
  lesser = compareResult ? leftElement : rightElement;   
  
  input_ptr[leftId*9 + 0] = lesser[0]; 
  input_ptr[leftId*9 + 1] = lesser[1];
  input_ptr[leftId*9 + 2] = lesser[2];
  input_ptr[leftId*9 + 3] = lesser[3];
  input_ptr[leftId*9 + 4] = lesser[4];
  input_ptr[leftId*9 + 5] = lesser[5];
  input_ptr[leftId*9 + 6] = lesser[6];
  input_ptr[leftId*9 + 7] = lesser[7];
  input_ptr[leftId*9 + 8] = lesser[8];
  input_ptr[rightId*9 + 0] = greater[0];
  input_ptr[rightId*9 + 1] = greater[1]; 
  input_ptr[rightId*9 + 2] = greater[2]; 
  input_ptr[rightId*9 + 3] = greater[3]; 
  input_ptr[rightId*9 + 4] = greater[4]; 
  input_ptr[rightId*9 + 5] = greater[5]; 
  input_ptr[rightId*9 + 6] = greater[6]; 
  input_ptr[rightId*9 + 7] = greater[7]; 
  input_ptr[rightId*9 + 8] = greater[8]; 

} 


int main()
{
  unsigned int stage, passOfStage, numStages, temp;
  stage = passOfStage = numStages = 0;
  srand(time(NULL));
  float *iarry = (float*) malloc(sizeof(float)*SIZE*9); 
  float *padd_arry;

  for(int i = 0; i < SIZE*9; ++i)
  {
	iarry[i] = (float) (rand() >> 22);
	printf("%d: %f\n", i, iarry[i]);
  }
   //int twos_padd_reallocf(
//	int * input_ptr,
//	int* out_ptr,
//	size_t size)
//{
  unsigned int n = SIZE-1;
  unsigned int p2 = 0;
  size_t padded_size;
  do ++p2; while( (n >>= 0x1) != 0);
  padded_size = 0x1 << p2; 
  padd_arry = (float*) malloc( padded_size*9*sizeof(float));
  
  if(padd_arry)
  {
	printf("in loop\n");
	for( int i = 0; i < SIZE*9; ++i)
	  padd_arry[i] = iarry[i];

	for(int i = SIZE*9; i < padded_size*9; ++i)
	{
	  padd_arry[i] = -1;
	}
	//return new_size;
  }
  
  printf("done padding\n");
    for(temp = padded_size; temp > 1; temp >>= 1)
        ++numStages;

  printf("numstages: %d", numStages); 
  for(stage = 0; stage < numStages; ++stage)
  {
	// Every stage has stage + 1 passes
	for(passOfStage = 0; passOfStage < stage + 1; 
		++passOfStage) 
	{
	  for(int i = 0; i < padded_size; ++i)
		bitonic_sort(padd_arry, stage, passOfStage, i);
    }
	printf("stage: %d\n", stage);

  }
  printf("done!\n");

  for(int i = 0; i < padded_size*9; i+=9)
	printf("%f\n", padd_arry[i+2]);

  return 0;
}
