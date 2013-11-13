// Simple sort    for (int it=1;it<=1<<20 && ok;it<<=1)

// EB Jun 2011

#include <MiniCL.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <list>
#include <algorithm>
#include "TestFunctions.h"

// 0 = sort key vector
// 1 = sort key + value vector
#define CONFIG_USE_VALUE 0

// Select tests to run (set to 0 or 1)
#define TEST_PARALLEL_SELECTION       1
#define TEST_PARALLEL_SELECTION_BLOCK 1
#define TEST_PARALLEL_SELECTION_LOCAL 1
#define TEST_PARALLEL_MERGE_LOCAL     1
#define TEST_PARALLEL_BITONIC_LOCAL   1
#define TEST_PARALLEL_BITONIC_A       1
#define TEST_PARALLEL_BITONIC_B       1
#define TEST_PARALLEL_BITONIC_C       1

// Allowed "Bx" kernels (bit mask)
#define ALLOWB (2+4+8)

#if CONFIG_USE_VALUE
typedef cl_uint2 data_t;
inline void setKey(data_t & a,cl_uint key) { a.s[0] = key; }
inline void setValue(data_t & a,cl_uint value) { a.s[1] = value; }
inline cl_uint getKey(const data_t & a) { return a.s[0]; }
inline cl_uint getValue(const data_t & a) { return a.s[1]; }
inline bool operator < (const data_t & a,const data_t & b) { return getKey(a) < getKey(b); }
inline bool operator == (const data_t & a,const data_t & b) { return (getKey(a) == getKey(b)) && (getValue(a) == getValue(b)); }
inline bool operator != (const data_t & a,const data_t & b) { return (getKey(a) != getKey(b)) || (getValue(a) != getValue(b)); }
#else
typedef cl_uint data_t;
inline void setKey(data_t & a,cl_uint key) { a = key; }
inline void setValue(data_t & a,cl_uint value) { }
inline cl_uint getKey(const data_t & a) { return a; }
inline cl_uint getValue(const data_t & a) { return 0; }
#endif
inline data_t makeData(cl_uint key,cl_uint value) { data_t a; setKey(a,key); setValue(a,value); return a; }

inline int log2(int n)
{
  int l;
  for (l=-1;n>0;n>>=1) l++;
  return l;
}

using namespace MINICL_NAMESPACE;
extern const char * OpenCLKernelSource;
enum Kernels {
  COPY_KERNEL,
  PARALLEL_SELECTION_KERNEL,
  PARALLEL_SELECTION_BLOCKS_KERNEL,
  PARALLEL_SELECTION_LOCAL_KERNEL,
  PARALLEL_MERGE_LOCAL_KERNEL,
  PARALLEL_BITONIC_LOCAL_KERNEL,
  PARALLEL_BITONIC_A_KERNEL,
  PARALLEL_BITONIC_B2_KERNEL,
  PARALLEL_BITONIC_B4_KERNEL,
  PARALLEL_BITONIC_B8_KERNEL,
  PARALLEL_BITONIC_B16_KERNEL,
  PARALLEL_BITONIC_C2_KERNEL,
  PARALLEL_BITONIC_C4_KERNEL,
  NB_KERNELS
};
const char * KernelNames[NB_KERNELS+1] = {
  "Copy","ParallelSelection","ParallelSelection_Blocks","ParallelSelection_Local", "ParallelMerge_Local", "ParallelBitonic_Local",
  "ParallelBitonic_A", "ParallelBitonic_B2", "ParallelBitonic_B4", "ParallelBitonic_B8", "ParallelBitonic_B16",
  "ParallelBitonic_C2", "ParallelBitonic_C4",
  0 };

// Sorting algorithm base class
class SortingAlgorithm
{
public:
  // Sort IN[N] into OUT[N] using the specified context and device
  // This function will be called between two clFinish, and timed.
  virtual bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const = 0;
  // Get total memory I/O of the algorithm for size N (Load + Store bytes)
  virtual double memoryIO(int n) const = 0;
  // Get additional OpenCL compilation options
  virtual void getOptions(std::string & options) const { options.clear(); }
  // Print algorithm identification on stdout
  virtual void printID() const { printf("Unidentified algorithm\n"); }
  // Check output. The base implementation compares OUT[N] to the sorted IN[N]
  virtual bool checkOutput(int n,const data_t * in,const data_t * out) const
  { return checkOutputFullSorted(n,in,out); }

protected:
  // Specific check functions

  // Check OUT[N] is IN[N] sorted
  bool checkOutputFullSorted(int n,const data_t * in,const data_t * out) const
  {
    bool ok = true;
    std::vector<data_t> expected(n,makeData(0,0));
    size_t sz = n * sizeof(data_t);
    memcpy(&(expected[0]),in,sz);
    std::sort(expected.begin(),expected.end());

    // Check result
    for (int i=0;i<n;i++)
    {
      if (getKey(out[i]) != getKey(expected[i]))
      {
        printf("Invalid output, I=%d\n",i);
        for (int j=i-10;j<=i+10;j++)
        {
          if (j<0 || j>=n) continue;
          printf("OUT[%3d] = %9u,%4u  EXPECTED[%3d] = %9u,%4u\n",j,getKey(out[j]),getValue(out[j]),j,getKey(expected[j]),getValue(expected[j]));
        }
        ok = false;
        break;
      }
    }

    return ok;
  }

  // Check OUT[N] is IN[N] sorted by blocks of size WG
  bool checkOutputBlockSorted(int n,int wg,const data_t * in,const data_t * out) const
  {
    bool ok = true;
    std::vector<data_t> expected(n,makeData(0,0));
    size_t sz = n * sizeof(data_t);
    memcpy(&(expected[0]),in,sz);
    for (int i=0;i<n;i+=wg)
    {
      std::sort(expected.begin()+i,expected.begin()+i+wg);
    }

    // Check result
    for (int i=0;i<n;i++)
    {
      if (getKey(out[i]) != getKey(expected[i]))
      {
        printf("Invalid output, I=%d\n",i);
        for (int j=i-10;j<=i+10;j++)
        {
          if (j<0 || j>=n) continue;
          printf("OUT[%3d] = %9u,%4u  EXPECTED[%3d] = %9u,%4u\n",j,getKey(out[j]),getValue(out[j]),j,getKey(expected[j]),getValue(expected[j]));
        }
        ok = false;
        break;
      }
    }

    return ok;
  }
};

class CopySort : public SortingAlgorithm
{
public:
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = COPY_KERNEL;
    c->clearArgs(kid);
    c->pushArg(kid,in);
    c->pushArg(kid,out);
    c->enqueueKernel(targetDevice,kid,n,1,256,1,EventVector());
    return true;
  }
  double memoryIO(int n) const { double x = (double)n; return 2*x*sizeof(data_t); }
  virtual void printID() const { printf("Copy\n"); }
};

/*
*/
class ParallelBitonicLocalSort : public SortingAlgorithm
{
public:
  ParallelBitonicLocalSort(int wg) : mWG(wg) { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = PARALLEL_BITONIC_LOCAL_KERNEL;
    if ( (n % mWG) != 0 ) return false; // Invalid
    c->clearArgs(kid);
    c->pushArg(kid,in);
    c->pushArg(kid,out);
    c->pushLocalArg(kid,sizeof(data_t)*mWG);
    c->enqueueKernel(targetDevice,kid,n,1,mWG,1,EventVector());
    return true;
  }
  double memoryIO(int n) const { double x = (double)n; return (x*(double)mWG+2*x)*sizeof(data_t); }
  bool checkOutput(int n,const data_t * in,const data_t * out) const { return checkOutputBlockSorted(n,mWG,in,out); }
  void printID() const { printf("Parallel bitonic, local, WG=%d\n",mWG); }
private:
  int mWG;
};

/*
*/

// Test a sorting algorithm
bool testSortingAlgorithm(int maxN,const SortingAlgorithm & algo)
{
  // Setup OpenCL
  cl_context clContext = createGPUContext(false);
  if (clContext == 0) { printf("Context creation failed\n"); exit(1); }
  std::string errorMsg;
  char options[2000];
  std::string algoOptions;
  algo.getOptions(algoOptions);
  snprintf(options,2000,"-cl-fast-relaxed-math -D CONFIG_USE_VALUE=%d %s ",CONFIG_USE_VALUE,algoOptions.c_str());
  Context * c = Context::create(clContext,OpenCLKernelSource,options,KernelNames,errorMsg);
  clReleaseContext(clContext);
  if (c == 0) { printf("%s\n",errorMsg.c_str()); exit(1); }
  int targetDevice = c->getNDevices() - 1; // Run on last available device (assuming the X server is running on the first device)
  //printf("Initialization OK [%s] targetDevice=%d\n",options,targetDevice);
  printf("____________________________________________________________\n");
#if CONFIG_USE_VALUE
  printf("Key+Value / ");
#else
  printf("Key / ");
#endif
  algo.printID();
  
  // Setup test vector
  data_t * a = new data_t[maxN];
  data_t * b = new data_t[maxN];
  for (int i=0;i<maxN;i++)
  {

/* PROBABLY NOT THE BEST WAY TO MAKE RAND FLOATS */
#if 1
    cl_uint x = (cl_uint)0;
    x = (x << 14) | ((cl_uint)rand() & 0x3FFF);
    x = (x << 14) | ((cl_uint)rand() & 0x3FFF);
#else
    cl_uint x = (cl_uint)(maxN - i);
#endif
    setKey(a[i],x);
    setValue(a[i],(cl_uint)i);
  }

  bool ok = true;
  for (int n = 256; n <= maxN && ok; n <<= 1)
  {
    size_t sz = n * sizeof(data_t);
    cl_mem inBuffer = c -> createBuffer(CL_MEM_READ_ONLY,sz,0);
    cl_mem outBuffer = c -> createBuffer(CL_MEM_READ_WRITE,sz,0);

    double bdt = getRealTime();
    Event e;
    double t0 = getRealTime();
    e = c->enqueueWrite(targetDevice,inBuffer,true,0,sz,a,EventVector()); // blocking
    c->finish(targetDevice);

    double dt = 0;
    double nit = 0;
    for (int it = 1; it <= 1 << 20 && ok; it <<= 1)
    {
      for (int i = 0; i < it && ok; i++)
      {
        ok &= algo.sort(c,targetDevice,n,inBuffer,outBuffer);
        c->finish(targetDevice);
      }
      dt = getRealTime() - t0; 
      nit += (double)it;
      if (dt > 0.5) break; // min time
    }
    if (!ok) { ok = true; continue; } // ignore launch errors
    dt /= nit;

    e = c->enqueueRead(targetDevice,outBuffer,true,0,sz,b,EventVector()); // blocking
    double u = 1.0e-6 * (double)n/dt;
    bdt = getRealTime() - bdt;
    printf("N=2^%d  R=%.2f Mkeys/s  dt:%f  bdt:%f\n",log2(n),u,dt,bdt);

   ok &= algo.checkOutput(n,a,b);
#if 0
    // Check debug output is all 0
    for (int i=0;i<n;i++)
    {
      if (getKey(b[i]) != 0)
      {
        printf("Non-zero value, I=%d\n",i);
        for (int j=i-10;j<=i+10;j++)
        {
          if (j<0 || j>=n) continue;
          printf("OUT[%d] = %u\n",j,b[j]);
        }
        ok = false;
        break;
      }
    }
#endif
    clReleaseMemObject(inBuffer);
    clReleaseMemObject(outBuffer);

    if (dt > 4.0 || !ok) break; // Too long
  }

  delete [] a;
  delete [] b;
  delete c;

  return ok;
}

int main(int argc,char ** argv)
{
  srand((unsigned int)time(0));
  bool ok = true;
  int maxN = 1<<25;

#if TEST_PARALLEL_BITONIC_LOCAL
  for (int wg=32;wg<=256 && ok;wg<<=1)
  {
    ok &= testSortingAlgorithm(maxN,ParallelBitonicLocalSort(wg));
  }
#endif

  return 0;
}




