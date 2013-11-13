// OpenCL FP test
// EB May 2011

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include "MiniCL.h"
#include "TestFunctions.h"

using namespace MINICL_NAMESPACE;

const char * KernelNames[] = { "f", 0 };

int main()
{
  bool ok = true;
  srand(0);

  cl_context context = createGPUContext(true);
  if (context == 0)
  {
    fprintf(stderr,"Could not create OpenCL context\n");
    return false;
  }

  std::string errorMsg;
  Context * c = Context::create(context,
				"__kernel void f(__global float * x) { x[get_global_id(0)] = 0.0f; }\n",
				"-cl-fast-relaxed-math",
				KernelNames,
				errorMsg);
  if (c == 0)
    {
      fprintf(stderr,"CLFP Context creation failed:\n%s\n",errorMsg.c_str());
      ok = false;
    }

  delete c;

  printf("%s\n",(ok)?"OK":"FAILED!");
#ifdef WIN32
  printf("Press a key.\n");
  getchar();
#endif
}
