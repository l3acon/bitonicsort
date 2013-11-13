#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string>
#include <CL/cl.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef Linux
#include <sys/time.h>
#endif

#include "TestFunctions.h"

#ifdef WIN32
double getRealTime()
{
  LARGE_INTEGER freq,value;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&value);
  return (double)value.QuadPart/(double)freq.QuadPart;
}
#endif

#ifdef Linux
double getRealTime()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return (double)tv.tv_sec + 1.0e-6*(double)tv.tv_usec;
}
#endif

// Return random float in 0..1
float rnd()

{
  float s = 0;
#if 0
  const double k = 1.0/(1.0+RAND_MAX);
  s = k * (s + (double)rand());
  s = k * (s + (double)rand());
#else
  s = rand() & 15;  // to easily test partial sums
#endif
  return s;
}
