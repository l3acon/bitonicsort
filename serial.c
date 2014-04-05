#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 27

typedef struct vertex_t {
  float x1, y1, z1;
  float x2, y2, z2;
  float x3, y3, z3;
} Vertex;

void bitonic_sort(                          		  
            Vertex *input_ptr,                   	 
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
       
  Vertex leftElement, rightElement;    
  Vertex *greater, *lesser;    
  
  leftElement.x1 = input_ptr[leftId].x1;   
  leftElement.y1 = input_ptr[leftId].y1;   
  leftElement.z1 = input_ptr[leftId].z1;   
  leftElement.x2 = input_ptr[leftId].x2;   
  leftElement.y2 = input_ptr[leftId].y2;   
  leftElement.z2 = input_ptr[leftId].z2;   
  leftElement.x3 = input_ptr[leftId].x3;   
  leftElement.y3 = input_ptr[leftId].y3;   
  leftElement.z3 = input_ptr[leftId].z3;   

  rightElement.x1 = input_ptr[rightId].x1;  
  rightElement.y2 = input_ptr[rightId].y1;  
  rightElement.z2 = input_ptr[rightId].z1;  
  rightElement.x2 = input_ptr[rightId].x1;  
  rightElement.y1 = input_ptr[rightId].y1;  
  rightElement.z1 = input_ptr[rightId].z1;  
  rightElement.x1 = input_ptr[rightId].x1;  
  rightElement.y1 = input_ptr[rightId].y1;  
  rightElement.z1 = input_ptr[rightId].z1;  

  unsigned int sameDirectionBlockWidth = threadId >> stage;      
  unsigned int sameDirection = sameDirectionBlockWidth & 0x1;   
     
  temp = sameDirection ? rightId : temp;   
  rightId = sameDirection ? leftId : rightId;   
  leftId = sameDirection ? temp : leftId;   
     
  compareResult = (leftElement.z1 < rightElement.z1);   
     
  greater = compareResult ? &rightElement : &leftElement;   
  lesser = compareResult ? &leftElement : &rightElement;   
  
  input_ptr[leftId].x1 = lesser->x1;   
  input_ptr[leftId].y1 = lesser->y1;   
  input_ptr[leftId].z1 = lesser->z1;   
  input_ptr[leftId].x2 = lesser->x2;   
  input_ptr[leftId].y2 = lesser->y2;   
  input_ptr[leftId].z2 = lesser->z2;   
  input_ptr[leftId].x3 = lesser->x3;   
  input_ptr[leftId].y3 = lesser->y3;   
  input_ptr[leftId].z3 = lesser->z3;   

  input_ptr[rightId].x1 = greater->x1;   
  input_ptr[rightId].y1 = greater->y1;   
  input_ptr[rightId].z1 = greater->z1;   
  input_ptr[rightId].x2 = greater->x2;   
  input_ptr[rightId].y2 = greater->y2;   
  input_ptr[rightId].z2 = greater->z2;   
  input_ptr[rightId].x3 = greater->x3;   
  input_ptr[rightId].y3 = greater->y3;   
  input_ptr[rightId].z3 = greater->z3;   

} 


int main()
{
  unsigned int stage, passOfStage, numStages, temp;
  stage = passOfStage = numStages = 0;
  srand(time(NULL));
  Vertex *iarry = (Vertex*) malloc(sizeof(Vertex)*SIZE); 
  Vertex *padd_arry;

  for(int i = 0; i < SIZE; ++i)
  {
	iarry[i].x1 = (float) (rand() >> 22);
	iarry[i].y1 = (float) (rand() >> 22);
	iarry[i].z1 = (float) (rand() >> 22);
	iarry[i].x2 = (float) (rand() >> 22);
	iarry[i].y2 = (float) (rand() >> 22);
	iarry[i].z2 = (float) (rand() >> 22);
	iarry[i].x3 = (float) (rand() >> 22);
	iarry[i].y3 = (float) (rand() >> 22);
	iarry[i].z3 = (float) (rand() >> 22);
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
  padd_arry = (Vertex*) malloc( padded_size*sizeof(Vertex));
  
  if(padd_arry)
  {
	printf("in loop\n");
	for( int i = 0; i < SIZE; ++i)
	  padd_arry[i] = iarry[i];

	for(int i = SIZE; i < padded_size; ++i)
	{
	  padd_arry[i].x1 = -1;
	  padd_arry[i].y1 = -1;
	  padd_arry[i].z1 = -1;
	  padd_arry[i].x2 = -1;
	  padd_arry[i].y2 = -1;
	  padd_arry[i].z2 = -1;
	  padd_arry[i].x3 = -1;
	  padd_arry[i].y3 = -1;
	  padd_arry[i].z3 = -1;
	}
	//return new_size;
  }
  printf("done padding\n");
  //else
	//return -1;
//}
  //padded_size = twos_padd_reallocf(iarry, padd_arry, SIZE);
  //for(int i = 0; i < padded_size; ++i)
  //{
  //  printf("%d: %f \n", i, padd_arry[i].x1);
  //  printf("%d: %f \n", i, padd_arry[i].y1);
  //  printf("%d: %f \n", i, padd_arry[i].z1);
  //  printf("%d: %f \n", i, padd_arry[i].x2);
  //  printf("%d: %f \n", i, padd_arry[i].y2);
  //  printf("%d: %f \n", i, padd_arry[i].z2);
  //  printf("%d: %f \n", i, padd_arry[i].x3);
  //  printf("%d: %f \n", i, padd_arry[i].y3);
  //  printf("%d: %f \n", i, padd_arry[i].z3);
  //}

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

  for(int i = 0; i < padded_size; ++i)
	printf("%f\n", padd_arry[i].z1);

  return 0;
}
