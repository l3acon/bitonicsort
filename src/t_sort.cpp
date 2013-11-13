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
class ParallelSelectionSort : public SortingAlgorithm
{
public:
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = PARALLEL_SELECTION_KERNEL;
    int wg = std::min(n,256);
    c->clearArgs(kid);
    c->pushArg(kid,in);
    c->pushArg(kid,out);
    c->enqueueKernel(targetDevice,kid,n,1,wg,1,EventVector());
    return true;
  }
  double memoryIO(int n) const { double x = (double)n; return (x*x+2*x)*sizeof(data_t); }
  virtual void printID() const { printf("Parallel selection\n"); }
};

class ParallelSelectionBlocksSort : public SortingAlgorithm
{
public:
  ParallelSelectionBlocksSort(int blockFactor) : mBlockFactor(blockFactor) { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = PARALLEL_SELECTION_BLOCKS_KERNEL;
    int wg = 256;
    if ( (n % (wg * mBlockFactor)) != 0 ) return false; // Invalid
    c->clearArgs(kid);
    c->pushArg(kid,in);
    c->pushArg(kid,out);
    c->pushLocalArg(kid,sizeof(cl_uint)*wg*mBlockFactor);
    c->enqueueKernel(targetDevice,kid,n,1,wg,1,EventVector());
    return true;
  }
  double memoryIO(int n) const { double x = (double)n; return (x*x+2*x)*sizeof(data_t); }
  void getOptions(std::string & options) const
  {
    char aux[200]; snprintf(aux,200," -D BLOCK_FACTOR=%d ",mBlockFactor);
    options.assign(aux);
  }
  virtual void printID() const { printf("Parallel selection, blocks, BLOCK_FACTOR=%d\n",mBlockFactor); }
private:
  int mBlockFactor;
};

class ParallelSelectionLocalSort : public SortingAlgorithm
{
public:
  ParallelSelectionLocalSort(int wg) : mWG(wg) { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = PARALLEL_SELECTION_LOCAL_KERNEL;
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
  void printID() const { printf("Parallel selection, local, WG=%d\n",mWG); }
private:
  int mWG;
};

class ParallelMergeLocalSort : public SortingAlgorithm
{
public:
  ParallelMergeLocalSort(int wg) : mWG(wg) { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int kid = PARALLEL_MERGE_LOCAL_KERNEL;
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
  void printID() const { printf("Parallel merge, local, WG=%d\n",mWG); }
private:
  int mWG;
};
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
class ParallelBitonicASort : public SortingAlgorithm
{
public:
  ParallelBitonicASort() { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    int nk = 0;

    int kid = PARALLEL_BITONIC_A_KERNEL;
    cl_mem buffers[2];
    buffers[0] = in;
    buffers[1] = out;
    int current = 0;
    for (int length=1;length<n;length<<=1) for (int inc=length;inc>0;inc>>=1)
    {
      c->clearArgs(kid);
      c->pushArg(kid,buffers[current]);
      c->pushArg(kid,buffers[1-current]);
      c->pushArg(kid,inc);
      c->pushArg(kid,length<<1);
      c->enqueueKernel(targetDevice,kid,n,1,256,1,EventVector());
      c->enqueueBarrier(targetDevice); // sync
      current = 1-current;
      nk++;
    }
    // printf("n=2^%d nk=%d\n",log2(n),nk);
    return (current == 1);  // output must be in OUT
  }
  double memoryIO(int n) const
  {
    double x = (double)n;
    double l = (double)log2(n);
    return (3*(l*(l+1))/2)*x*sizeof(data_t);
  }
  bool checkOutput(int n,const data_t * in,const data_t * out) const { return checkOutputFullSorted(n,in,out); }
  void printID() const { printf("Parallel bitonic A\n"); }
};
*/

/*
class ParallelBitonicBSort : public SortingAlgorithm
{
public:
  ParallelBitonicBSort() { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    c->enqueueCopy(targetDevice,in,out,0,0,n*sizeof(data_t),EventVector());
    c->enqueueBarrier(targetDevice); // sync

    cl_mem buffers[2];
    buffers[0] = in;
    buffers[1] = out;
    for (int length=1;length<n;length<<=1)
    {
      int inc = length;
      while (inc > 0)
      {
        int ninc = 0;
        int kid;
#if (ALLOWB & 16)  // Allow B16
        if (inc >= 8 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B16_KERNEL;
          ninc = 4;
        }
#endif
#if (ALLOWB & 8)  // Allow B8
        if (inc >= 4 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B8_KERNEL;
          ninc = 3;
        }
#endif
#if (ALLOWB & 4)  // Allow B4
        if (inc >= 2 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B4_KERNEL;
          ninc = 2;
        }
#endif
        // Always allow B2
        if (ninc == 0)
        {
          kid = PARALLEL_BITONIC_B2_KERNEL;
          ninc = 1;
        }
        int nThreads = n >> ninc;
        int wg = c->getMaxWorkgroupSize(targetDevice,kid);
        wg = std::min(wg,256);
        wg = std::min(wg,nThreads);
        c->clearArgs(kid);
        c->pushArg(kid,out);
        c->pushArg(kid,inc); // INC passed to kernel
        c->pushArg(kid,length<<1); // DIR passed to kernel
        c->enqueueKernel(targetDevice,kid,nThreads,1,wg,1,EventVector());
        c->enqueueBarrier(targetDevice); // sync
        inc >>= ninc;
      }
    }
    return true;
  }
  double memoryIO(int n) const
  {
    double x = (double)n;
    double l = (double)log2(n);
    double nk = (l*(l+1))/2; nk /= 2;
    return 2*nk*x*sizeof(data_t);
  }
  bool checkOutput(int n,const data_t * in,const data_t * out) const { return checkOutputFullSorted(n,in,out); }
  void printID() const {
    printf("Parallel bitonic B2");
#if (ALLOWB & 4)
    printf("+B4");
#endif
#if (ALLOWB & 8)
    printf("+B8");
#endif
#if (ALLOWB & 16)
    printf("+B16");
#endif
    printf("\n");
  }
};
*/


/*
class ParallelBitonicCSort : public SortingAlgorithm
{
public:
  ParallelBitonicCSort() : mLastN(-1) { }
  bool sort(Context * c,int targetDevice,int n,cl_mem in,cl_mem out) const
  {
    c->enqueueCopy(targetDevice,in,out,0,0,n*sizeof(data_t),EventVector());
    c->enqueueBarrier(targetDevice); // sync

    cl_mem buffers[2];
    buffers[0] = in;
    buffers[1] = out;
    for (int length=1;length<n;length<<=1)
    {
      int inc = length;
      std::list<int> strategy; // vector defining the sequence of reductions
      {
        int ii = inc;
        while (ii>0)
        {
          if (ii==128 || ii==32 || ii==8) { strategy.push_back(-1); break; } // C kernel
          int d = 1; // default is 1 bit
          if (0) d = 1;
#if 1
          // Force jump to 128
          else if (ii==256) d = 1;
          else if (ii==512 && (ALLOWB & 4)) d = 2;
          else if (ii==1024 && (ALLOWB & 8)) d = 3;
          else if (ii==2048 && (ALLOWB & 16)) d = 4;
#endif
          else if (ii>=8 && (ALLOWB & 16)) d = 4;
          else if (ii>=4 && (ALLOWB & 8)) d = 3;
          else if (ii>=2 && (ALLOWB & 4)) d = 2;
          else d = 1;
          strategy.push_back(d);
          ii >>= d;
        }
      }

      while (inc > 0)
      {
        int ninc = 0;
        int kid = -1;
        int doLocal = 0;
        int nThreads = 0;
        int d = strategy.front(); strategy.pop_front();

        switch (d)
        {
        case -1:
          kid = PARALLEL_BITONIC_C4_KERNEL;
          ninc = -1; // reduce all bits
          doLocal = 4;
          nThreads = n >> 2;
          break;
        case 4:
          kid = PARALLEL_BITONIC_B16_KERNEL;
          ninc = 4;
          nThreads = n >> ninc;
          break;
        case 3:
          kid = PARALLEL_BITONIC_B8_KERNEL;
          ninc = 3;
          nThreads = n >> ninc;
          break;
        case 2:
          kid = PARALLEL_BITONIC_B4_KERNEL;
          ninc = 2;
          nThreads = n >> ninc;
          break;
        case 1:
          kid = PARALLEL_BITONIC_B2_KERNEL;
          ninc = 1;
          nThreads = n >> ninc;
          break;
        default:
          printf("Strategy error!\n");
          break;
        }
        int wg = c->getMaxWorkgroupSize(targetDevice,kid);
        wg = std::min(wg,256);
        wg = std::min(wg,nThreads);
        c->clearArgs(kid);
        c->pushArg(kid,out);
        c->pushArg(kid,inc); // INC passed to kernel
        c->pushArg(kid,length<<1); // DIR passed to kernel
        if (doLocal>0) c->pushLocalArg(kid,doLocal*wg*sizeof(data_t)); // DOLOCAL values / thread
        c->enqueueKernel(targetDevice,kid,nThreads,1,wg,1,EventVector());
        c->enqueueBarrier(targetDevice); // sync
        // if (mLastN != n) printf("LENGTH=%d INC=%d KID=%d\n",length,inc,kid); // DEBUG
        if (ninc < 0) break; // done
        inc >>= ninc;
      }
    }
    mLastN = n;
    return true;
  }

  double memoryIO(int n) const
  {
    double x = (double)n;
    double l = (double)log2(n);
    double nk = (l*(l+1))/2; nk /= 2;
    return 2*nk*x*sizeof(data_t);
  }
  bool checkOutput(int n,const data_t * in,const data_t * out) const { return checkOutputFullSorted(n,in,out); }
  void printID() const {
    printf("Parallel bitonic C4+B2");
#if (ALLOWB & 4)
    printf("+B4");
#endif
#if (ALLOWB & 8)
    printf("+B8");
#endif
#if (ALLOWB & 16)
    printf("+B16");
#endif
    printf("\n");
  }
private:
  mutable int mLastN;
};
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

/*	other testing down here
#if TEST_PARALLEL_SELECTION
  ok &= testSortingAlgorithm(maxN,ParallelSelectionSort());
#endif
#if TEST_PARALLEL_SELECTION_BLOCK
  for (int b=1;b<=32 && ok;b<<=1)
  {
    ok &= testSortingAlgorithm(maxN,ParallelSelectionBlocksSort(b));
  }
#endif
#if TEST_PARALLEL_SELECTION_LOCAL
  for (int wg=1;wg<=256 && ok;wg<<=1)
  {
    ok &= testSortingAlgorithm(maxN,ParallelSelectionLocalSort(wg));
  }
#endif
#if TEST_PARALLEL_MERGE_LOCAL
  for (int wg=1;wg<=256 && ok;wg<<=1)
  {
    ok &= testSortingAlgorithm(maxN,ParallelMergeLocalSort(wg));
  }
#endif
*/
#if TEST_PARALLEL_BITONIC_LOCAL
  for (int wg=32;wg<=256 && ok;wg<<=1)
  {
    ok &= testSortingAlgorithm(maxN,ParallelBitonicLocalSort(wg));
  }

#endif
/*
#if TEST_PARALLEL_BITONIC_A
  ok &= testSortingAlgorithm(maxN,ParallelBitonicASort());
#endif
#if TEST_PARALLEL_BITONIC_B
  ok &= testSortingAlgorithm(maxN,ParallelBitonicBSort());
#endif
#if TEST_PARALLEL_BITONIC_C
  ok &= testSortingAlgorithm(maxN,ParallelBitonicCSort());
#endif

*/  
  return 0;
}
