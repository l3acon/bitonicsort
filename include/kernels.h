#ifndef KERNELS_H
#define KERNELS_H

typedef struct vertex_t 
{
  float x1, y1, z1;
  float x2, y2, z2;
  float x3, y3, z3;
} Vertex;


const char * bitonic_STL_sort_source  =
"\n typedef struct vertex_t 						"
"\n {												"
"\n  float x1, y1, z1;								"
"\n  float x2, y2, z2;								"
"\n  float x3, y3, z3;								"
"\n } Vertex;										"
"\n 												"
"__kernel                                           "
"\n void _kbitonic_stl_sort(                    	"
"\n             __global Vertex *input_ptr,        	"
"\n             const unsigned int stage,           "
"\n             const int passOfStage)              "
"\n {                                    			"
"\n                                      			"
"\n      unsigned int  threadId = get_global_id(0);  					"
"\n      unsigned int  pairDistance = 1 << (stage - passOfStage);   	"
"\n      unsigned int  blockWidth = 2 * pairDistance;    				"
"\n      unsigned int  temp;  											"
"\n                                                          			"
"\n      int compareResult;                                      		"
"\n      unsigned int  leftId = (threadId & (pairDistance - 1)) + 		" 
"			(threadId >> (stage - passOfStage) ) * blockWidth;  		" 
"\n      unsigned int  rightId = leftId + pairDistance;  				" 
"\n        																" 
"\n      Vertex leftElement, rightElement;  							" 
"\n    	 Vertex *greater, *lesser;  									" 
"\n 																	"
"\n		leftElement.x1 = input_ptr[leftId].x1;	"
"\n		leftElement.y1 = input_ptr[leftId].y1;	"
"\n		leftElement.z1 = input_ptr[leftId].z1;	"
"\n		leftElement.x2 = input_ptr[leftId].x2;	"
"\n		leftElement.y2 = input_ptr[leftId].y2;	"
"\n		leftElement.z2 = input_ptr[leftId].z2;	"
"\n		leftElement.x3 = input_ptr[leftId].x3;	"
"\n		leftElement.y3 = input_ptr[leftId].y3;	"
"\n		leftElement.z3 = input_ptr[leftId].z3;	"

"\n 	rightElement.x1 = input_ptr[rightId].x1;  "
"\n 	rightElement.y2 = input_ptr[rightId].y1;  "
"\n 	rightElement.z2 = input_ptr[rightId].z1;  "
"\n 	rightElement.x2 = input_ptr[rightId].x1;  "
"\n 	rightElement.y1 = input_ptr[rightId].y1;  "
"\n 	rightElement.z1 = input_ptr[rightId].z1;  "
"\n 	rightElement.x1 = input_ptr[rightId].x1;  "
"\n 	rightElement.y1 = input_ptr[rightId].y1;  "
"\n 	rightElement.z1 = input_ptr[rightId].z1;  "
"\n      																" 
"\n      unsigned int sameDirectionBlockWidth = threadId >> stage;   	" 
"\n      unsigned int sameDirection = sameDirectionBlockWidth & 0x1; 	" 
"\n      																" 
"\n      temp = sameDirection ? rightId : temp; 						" 
"\n      rightId = sameDirection ? leftId : rightId; 					" 
"\n      leftId = sameDirection ? temp : leftId;						" 
"\n       																" 
"\n      compareResult = (leftElement.z1 < rightElement.z1); 			" 
"\n       																" 
"\n      greater = compareResult ? &rightElement : &leftElement; 		" 
"\n      lesser = compareResult ? &leftElement : &rightElement; 		" 
"\n       																" 
"\n 	input_ptr[leftId].x1 = lesser->x1;   "
"\n 	input_ptr[leftId].y1 = lesser->y1;   "
"\n 	input_ptr[leftId].z1 = lesser->z1;   "
"\n 	input_ptr[leftId].x2 = lesser->x2;   "
"\n 	input_ptr[leftId].y2 = lesser->y2;   "
"\n 	input_ptr[leftId].z2 = lesser->z2;   "
"\n 	input_ptr[leftId].x3 = lesser->x3;   "
"\n 	input_ptr[leftId].y3 = lesser->y3;   "
"\n 	input_ptr[leftId].z3 = lesser->z3;   "
"\n 	input_ptr[rightId].x1 = greater->x1;   "
"\n 	input_ptr[rightId].y1 = greater->y1;   "
"\n 	input_ptr[rightId].z1 = greater->z1;   "
"\n 	input_ptr[rightId].x2 = greater->x2;   "
"\n 	input_ptr[rightId].y2 = greater->y2;   "
"\n 	input_ptr[rightId].z2 = greater->z2;   "
"\n 	input_ptr[rightId].x3 = greater->x3;   "
"\n 	input_ptr[rightId].y3 = greater->y3;   "
"\n 	input_ptr[rightId].z3 = greater->z3;   "
"\n }     										" 
; 



const char * bitonic_sort_kernel_source  =
"__kernel                                                    "
"\n void _kbitonic_sort_kernel(                          		 "
"\n             __global int *input_ptr,                   	"
"\n             const unsigned int stage,                   "
"\n             const int passOfStage)                       "
"\n {                                       				 "
"\n                                         				 "
"\n      unsigned int  threadId = get_global_id(0);  				 "
"\n      unsigned int  pairDistance = 1 << (stage - passOfStage);   "
"\n      unsigned int  blockWidth = 2 * pairDistance;    			"
"\n      unsigned int  temp;  										"
"\n                                                          		 "
"\n      int compareResult;                                      	"
"\n      unsigned int  leftId = (threadId & (pairDistance - 1)) + (threadId >> (stage - passOfStage) ) * blockWidth;  " 
"\n      unsigned int  rightId = leftId + pairDistance;  				 " 
"\n        																 " 
"\n      int leftElement, rightElement;  								" 
"\n    	 int greater, lesser;  											" 
"\n      leftElement = input_ptr[leftId]; 								" 
"\n      rightElement = input_ptr[rightId];								 " 
"\n      																 " 
"\n      unsigned int sameDirectionBlockWidth = threadId >> stage;   	 " 
"\n      unsigned int sameDirection = sameDirectionBlockWidth & 0x1; 	" 
"\n      																 " 
"\n      temp = sameDirection ? rightId : temp; 						" 
"\n      rightId = sameDirection ? leftId : rightId; 					" 
"\n      leftId = sameDirection ? temp : leftId;						 " 
"\n       																" 
"\n      compareResult = (leftElement < rightElement); 					" 
"\n       																" 
"\n      greater = compareResult ? rightElement : leftElement; 			" 
"\n      lesser = compareResult ? leftElement : rightElement; 			" 
"\n       																" 
"\n      input_ptr[leftId] = lesser; 									" 
"\n      input_ptr[rightId] = greater; 									" 
"\n }     																" 
; 
 

//The bitonic sort kernel does an ascending sort 
const char * original_kernel_source = 
" __kernel                                                      "
" \n void _kbitonic_sort_kernel(	                            "
" \n 			__global int * input_ptr,                       "
" \n     	const uint stage,                                   "
" \n     	const uint passOfStage )                            "
" \n {                                                          "
" \n     uint threadId = get_global_id(0);                      "
" \n     uint pairDistance = 1 << (stage - passOfStage);        "
" \n     uint blockWidth = 2 * pairDistance;                    "
" \n     uint temp;                                             "
" \n     bool compareResult;                                    "
" \n     uint leftId = (threadId & (pairDistance -1))           "
"          + (threadId >> (stage - passOfStage) ) * blockWidth; "
" \n     uint rightId = leftId + pairDistance;                  "
" \n                                                            "
" \n     DATA_TYPE leftElement, rightElement;                   "
" \n     DATA_TYPE greater, lesser;                             "
" \n     leftElement = input_ptr[leftId];                       "
" \n     rightElement = input_ptr[rightId];                     "
" \n                                                            "
" \n     uint sameDirectionBlockWidth = threadId >> stage;      "
" \n     uint sameDirection = sameDirectionBlockWidth & 0x1;    "
" \n                                                            "
" \n     temp = sameDirection ? rightId : temp;                 "
" \n     rightId = sameDirection	? leftId : rightId;         "
" \n     leftId = sameDirection ? temp : leftId;                "
" \n                                                            "
" \n     compareResult = (leftElement < rightElement) ;         "
" \n                                                            "
" \n     greater = compareResult ? rightElement : leftElement;  "
" \n     lesser= compareResult ? leftElement : rightElement;    "
" \n                                                            "
" \n     input_ptr[leftId] = lesser;                            "
" \n     input_ptr[rightId] = greater;                          "
" \n }                                                          "
;


#endif

