// MiniCL main include
// Copyright 2011, Eric Bainville

#ifndef MiniCL_h
#define MiniCL_h

#include <CL/cl.h>
#include <vector>
#include <string>
#include <stdio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#define MINICL_NAMESPACE mcl
#define BEGIN_MINICL_NAMESPACE namespace MINICL_NAMESPACE {
#define END_MINICL_NAMESPACE }
#define MINICL_CHECK_STATUS(status) MINICL_NAMESPACE::checkStatus(status,__FILE__,__LINE__)

BEGIN_MINICL_NAMESPACE

class Context;
class Event;
class EventVector;

const int NOpenCLErrorCodes = 63;
static const char * OpenCLErrorCodes[NOpenCLErrorCodes] = {
  "CL_SUCCESS", // 0
  "CL_DEVICE_NOT_FOUND", // -1
  "CL_DEVICE_NOT_AVAILABLE", // -2
  "CL_COMPILER_NOT_AVAILABLE", // -3
  "CL_MEM_OBJECT_ALLOCATION_FAILURE", // -4
  "CL_OUT_OF_RESOURCES", // -5
  "CL_OUT_OF_HOST_MEMORY", // -6
  "CL_PROFILING_INFO_NOT_AVAILABLE", // -7
  "CL_MEM_COPY_OVERLAP", // -8
  "CL_IMAGE_FORMAT_MISMATCH", // -9
  "CL_IMAGE_FORMAT_NOT_SUPPORTED", // -10
  "CL_BUILD_PROGRAM_FAILURE", // -11
  "CL_MAP_FAILURE", // -12
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // -13..-29
  "CL_INVALID_VALUE", // -30
  "CL_INVALID_DEVICE_TYPE", // -31
  "CL_INVALID_PLATFORM", // -32
  "CL_INVALID_DEVICE", // -33
  "CL_INVALID_CONTEXT", // -34
  "CL_INVALID_QUEUE_PROPERTIES", // -35
  "CL_INVALID_COMMAND_QUEUE", // -36
  "CL_INVALID_HOST_PTR", // -37
  "CL_INVALID_MEM_OBJECT", // -38
  "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR", // -39
  "CL_INVALID_IMAGE_SIZE", // -40
  "CL_INVALID_SAMPLER", // -41
  "CL_INVALID_BINARY", // -42
  "CL_INVALID_BUILD_OPTIONS", // -43
  "CL_INVALID_PROGRAM", // -44
  "CL_INVALID_PROGRAM_EXECUTABLE", // -45
  "CL_INVALID_KERNEL_NAME", // -46
  "CL_INVALID_KERNEL_DEFINITION", // -47
  "CL_INVALID_KERNEL", // -48
  "CL_INVALID_ARG_INDEX", // -49
  "CL_INVALID_ARG_VALUE", // -50
  "CL_INVALID_ARG_SIZE", // -51
  "CL_INVALID_KERNEL_ARGS", // -52
  "CL_INVALID_WORK_DIMENSION", // -53
  "CL_INVALID_WORK_GROUP_SIZE", // -54
  "CL_INVALID_WORK_ITEM_SIZE", // -55
  "CL_INVALID_GLOBAL_OFFSET", // -56
  "CL_INVALID_EVENT_WAIT_LIST", // -57
  "CL_INVALID_EVENT", // -58
  "CL_INVALID_OPERATION", // -59
  "CL_INVALID_GL_OBJECT", // -60
  "CL_INVALID_BUFFER_SIZE", // -61
  "CL_INVALID_MIP_LEVEL" // -62
};

/** Check OpenCL status value, and print error message if not success.
    Use MINICL_CHECK_STATUS(status) to call this function.

    @param status is the OpenCL value to check.
    @param filename,line are used in message.

    @return TRUE if status is CL_SUCCESS, and FALSE otherwise. */
inline bool checkStatus(cl_int status,const char * filename,int line)
{
  if (status == 0) return true; // OK
  int e = -status;
  if (e >= 0 && e < NOpenCLErrorCodes && OpenCLErrorCodes[e] != 0)
    fprintf(stderr,"%s:%d: OpenCL error %s\n",filename,line,OpenCLErrorCodes[e]);
  else
    fprintf(stderr,"%s:%d: OpenCL error %d\n",filename,line,-status);
  abort();
  return false; // Error
}

// Return log2(n) if N is a power of 2, and -1 otherwise. (internal function)
inline int log2(size_t n)
{
  int k = 0;
  if (n <= 0) return -1; // Not a power of 2
  while (n != (size_t)1)
  {
    if (n&(size_t)1) return -1; // Has at least 2 bits set
    n >>= (size_t)1;
    k++;
  }
  return k;
}

// Encapsulates one OpenCL event and the status of its creation.
class Event
{
public:

  // Constructor. Initialized to event 0 (invalid).
  inline Event() : mEvent(0), mStatus(CL_INVALID_EVENT) { }

  // Copy
  inline Event(const Event & e) : mEvent(0) { assign(e.mEvent); mStatus = e.mStatus; }

  // = operator
  inline Event & operator = (const Event & e) { assign(e.mEvent); mStatus = e.mStatus; return *this; }

  // Destructor, release the event.
  inline ~Event() { assign(0); }

  // Check if valid (non 0).
  inline bool isValid() const { return (mEvent != 0); }

  // Get OpenCL status returned by the function creating the event
  inline cl_int getStatus() const { return mStatus; }

private:

  // Constructor: takes ownership of E without incrementing reference count.
  // E may be 0. STATUS is the status returned by the function creating E.
  inline Event(cl_event e,cl_int status) : mEvent(e), mStatus(status) { }

  // Special case for E=0 (to return errors).
  inline explicit Event(cl_int status) : mEvent(0), mStatus(status) { }

  // Special case for STATUS=CL_SUCCESS
  inline explicit Event(cl_event e) : mEvent(e), mStatus(CL_SUCCESS) { }

  // Access event
  inline operator cl_event () const { return mEvent; }

  // Assign value E, release previous event if any, and retain E if not 0. E may be 0.
  inline void assign(cl_event e)
  {
    if (e == mEvent) return; // Nothing to do
    if (mEvent != 0) { clReleaseEvent(mEvent); mEvent = 0; }
    if (e != 0) { mEvent = e; clRetainEvent(mEvent); }
  }

  // Encapsulated event, friends only
  cl_event mEvent;
  // Status returned when the event was created (normally CL_SUCCESS if event is valid)
  cl_int mStatus;

  friend class MINICL_NAMESPACE::Context;
  friend class MINICL_NAMESPACE::EventVector;
}; // class Event

// Encapsulates a vector of valid OpenCL events
class EventVector
{
public:

  // Constructor. Initialize with the given events. Events are retained.
  inline EventVector() { }
  inline EventVector(Event & e1) { append(e1); }
  inline EventVector(Event & e1,Event & e2) { append(e1); append(e2); }
  inline EventVector(Event & e1,Event & e2,Event & e3) { append(e1); append(e2); append(e3); }

  // Destructor. Release the events.
  ~EventVector() { clear(); }

  // Append one event to the vector. Ignore if invalid. Otherwise the event is retained.
  inline void append(Event & e)
  {
    cl_event ce = (cl_event)e;
    if (ce == 0) return; // Invalid, ignore
    clRetainEvent(ce);
    mEvents.push_back(ce);
  }

  // Clear the vector. Release the events.
  inline void clear()
  {
    size_t n = mEvents.size();
    for (size_t i=0;i<n;i++) clReleaseEvent(mEvents[i]);
    mEvents.clear();
  }

private:

  // No copy (implement later if needed)
  EventVector(const EventVector &);
  EventVector & operator = (const EventVector &);

  // Access
  inline cl_uint size() const { return (cl_uint)mEvents.size(); }
  inline const cl_event * events() const { if (mEvents.empty()) return 0; else return &(mEvents[0]); }

  // Encapsulated events, friends only
  std::vector<cl_event> mEvents;

  friend class MINICL_NAMESPACE::Context;
};

// Encapsulates a context, command queues for all devices, and kernels
class Context
{
public:

  /** Create a new instance of the class, using the given OpenCL context.
      The object manages internally command queues and compile kernels for
      all devices attached to the context.

      @param context is the OpenCl context, we acquire a reference to it (shall be allocated by the caller).
      @param sourceCode is the OpenCL source code to compile.
      @param buildOptions are the build options passed to the OpenCL compiler.
      @param kernelNames contains the names of all kernels, the last one shall be 0.
      @param errorMsg receives error messages on failure, may be 0.

      @return a non 0 instance on success, 0 otherwise. */
  inline static Context * create(cl_context context,
				 const char * sourceCode,
				 const char * buildOptions,
				 const char ** kernelNames,
				 std::string & errorMsg)
  {
    errorMsg.clear();
    // Check args
    if (context == 0) { errorMsg.assign("Invalid OpenCL context"); return 0; }
    if (sourceCode == 0) { errorMsg.assign("Invalid source code"); return 0; }
    if (buildOptions == 0) { errorMsg.assign("Invalid build options"); return 0; }
    if (kernelNames == 0) { errorMsg.assign("Invalid kernel names"); return 0; }

    bool ok = true;
    cl_int status;
    const int MAX_DEVICES = 16;
    cl_device_id device[MAX_DEVICES];
    size_t sz;
    int nDevices;
    Context * result = 0;
    int nKernels = 0;
    while (kernelNames[nKernels] != 0) nKernels++;

    // Alloc result
    result = new Context();
    if (result == 0) return 0; // Alloc failed
    result->mContext = context;
    status = clRetainContext(context);
    if (!MINICL_CHECK_STATUS(status)) { ok = false; goto END; }

    // Get context devices
    status = clGetContextInfo(context,CL_CONTEXT_DEVICES,MAX_DEVICES*sizeof(device[0]),device,&sz);
    if (!MINICL_CHECK_STATUS(status)) { ok = false; goto END; }
    nDevices = (int) ( sz / sizeof(device[0]) );

    // Create and build program
    result->mProgram = clCreateProgramWithSource(context,1,&sourceCode,0,&status);
    if (!MINICL_CHECK_STATUS(status)) { ok = false; goto END; }
    status = clBuildProgram(result->mProgram,nDevices,device,buildOptions,0,0);
    // Collect build errors in msg string
    if (status == CL_BUILD_PROGRAM_FAILURE)
    {
      for (int d=0;d<nDevices;d++)
      {
        std::string e;
        cl_int s2;
        s2 = clGetProgramBuildInfo(result->mProgram,device[d],CL_PROGRAM_BUILD_LOG,0,0,&sz);
        if (!MINICL_CHECK_STATUS(s2) || sz == 0) continue;
        e.resize(sz+1,' ');
        s2 = clGetProgramBuildInfo(result->mProgram,device[d],CL_PROGRAM_BUILD_LOG,sz,&(e[0]),&sz);
        if (!MINICL_CHECK_STATUS(s2) || sz == 0) continue;
        e.resize(sz);
        while (sz > 0 && !isascii(e[sz-1])) sz--; // Eliminate garbage at the end of the string
        char aux[200];
        snprintf(aux,200,"Build error for device %d:\n",(int)d);
        errorMsg.append(aux);
        errorMsg.append(e);
      }
      ok = false; goto END;
    }
    if (!MINICL_CHECK_STATUS(status)) { ok = false; goto END; }

    // Create kernels
    result->mKernel.resize(nKernels,0);
    result->mKernelIndex.resize(nKernels,0);
    for (int i=0;i<nKernels;i++)
    {
      result->mKernel[i] = clCreateKernel(result->mProgram,kernelNames[i],&status);
      if (!MINICL_CHECK_STATUS(status)) { errorMsg.append("Kernel creation failed: "); errorMsg.append(kernelNames[i]); ok = false; goto END; }
    }

    // Create command queues
    result->mQueue.resize(nDevices,0);
    result->mDevice.resize(nDevices,0);
    for (int d=0;d<nDevices;d++)
    {
      cl_command_queue_properties props = 0;
      result->mDevice[d] = device[d];
      result->mQueue[d] = clCreateCommandQueue(context,device[d],props,&status);
      if (!MINICL_CHECK_STATUS(status)) { errorMsg.append("Command queue creation failed"); ok = false; goto END; }
    }

    // Keep max device workgroup size
    result->mMaxWorkGroupSize.resize(nDevices,(size_t)0);
    for (int d=0;d<nDevices;d++)
    {
      status = clGetDeviceInfo(result->mDevice[d],CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(size_t),&(result->mMaxWorkGroupSize[d]),0);
      if (!MINICL_CHECK_STATUS(status)) { errorMsg.append("MAX_WORK_GROUP_SIZE query failed"); ok = false; goto END; }
    }

END:
    if (!ok) { delete result; return 0; } // Error
    return result;
  }

  /** Destructor. */
  inline virtual ~Context()
  {
    for (std::vector<cl_command_queue>::iterator it = mQueue.begin(); it != mQueue.end(); it++) if (*it != 0) { clReleaseCommandQueue(*it); }
    for (std::vector<cl_kernel>::iterator it = mKernel.begin(); it != mKernel.end(); it++) if (*it != 0) { clReleaseKernel(*it); }
    mQueue.clear();
    mKernel.clear();
    mDevice.clear();
    if (mProgram != 0) clReleaseProgram(mProgram);
    if (mContext != 0) clReleaseContext(mContext);
  }

  /** Get number of devices and command queues attached to the creation context.

      @return number of devices/command queues. */
  inline int getNDevices() const { return (int)mDevice.size(); }

  /** Get OpenCL context.

      @return the OpenCL context passed at creation. */
  inline cl_context getOpenCLContext() const { return mContext; }

  /** Call clFinish on command queue.

      @param device is the device index (0 = first device of context, etc.).

      @return an OpenCL status. */
  inline cl_int finish(int device)
  {
    if (device < 0 || device >= (int)mDevice.size()) return CL_INVALID_DEVICE;
    return clFinish(mQueue[device]);
  }

  /** Enqueue a barrier in command queue.
   *
      @param device is the device index (0 = first device of context, etc.).

      @return an OpenCL status. */
/* DECLARED IN /AMDAPP/OPENCL
  inline cl_int enqueueBarrier(int device)
  {
    if (device < 0 || device >= (int)mDevice.size()) return CL_INVALID_DEVICE;
    return clEnqueueBarrier(mQueue[device]);
  }
*/
  /** Create a buffer.

      @param flags are the CL_MEM_XXX flags passed to clCreateBuffer.
      @param sz is the size of the buffer in bytes.
      @param hostPtr is the host pointer passed to clCreateBuffer, may be 0.

      @return a valid cl_mem object on success, and 0 otherwise. */
  inline cl_mem createBuffer(cl_mem_flags flags,size_t sz,void * hostPtr)
  {
    cl_int status;
    cl_mem m = clCreateBuffer(mContext,flags,sz,hostPtr,&status);
    if (!MINICL_CHECK_STATUS(status)) return 0;
    return m;
  }

  /** Enqueue a buffer read.

      @param device is the device index (0 = first device of context, etc.).
      @param buffer,blocking_read,offset,cb,ptr are arguments of clEnqueueReadBuffer.
      @param waitList is the event wait list.

      @return an OpenCL event. */
  inline Event enqueueRead(int device,cl_mem buffer,cl_bool blocking_read,size_t offset,size_t cb,void * ptr,const EventVector & waitList)
  {
    if (device < 0 || device >= (int)mDevice.size()) return Event(CL_INVALID_DEVICE);
    cl_event e;
    cl_int status;
    status = clEnqueueReadBuffer(mQueue[device],buffer,blocking_read,offset,cb,ptr,waitList.size(),waitList.events(),&e);
    if (!MINICL_CHECK_STATUS(status)) return Event(status);
    return Event(e,status);
  }

  /** Enqueue a buffer write.

      @param device is the device index (0 = first device of context, etc.).
      @param buffer,blocking_write,offset,cb,ptr are arguments of clEnqueueWriteBuffer.
      @param waitList is the event wait list.

      @return an OpenCL event. */
  inline Event enqueueWrite(int device,cl_mem buffer,cl_bool blocking_write,size_t offset,size_t cb,const void * ptr,const EventVector & waitList)
  {
    if (device < 0 || device >= (int)mDevice.size()) return Event(CL_INVALID_DEVICE);
    cl_event e;
    cl_int status;
    status = clEnqueueWriteBuffer(mQueue[device],buffer,blocking_write,offset,cb,ptr,waitList.size(),waitList.events(),&e);
    if (!MINICL_CHECK_STATUS(status)) return Event(status);
    return Event(e);
  }

  /** Enqueue a buffer copy
      @param device is the device index (0 = first device of context, etc.).
      @param other args are arguments of clEnqueueCopyBuffer.
      @param waitList is the event wait list.

      @return an OpenCL event. */
  inline Event enqueueCopy(int device,cl_mem srcBuffer,cl_mem dstBuffer,size_t srcOffset,size_t dstOffset,size_t cb,const EventVector & waitList)
  {
    if (device < 0 || device >= (int)mDevice.size()) return Event(CL_INVALID_DEVICE);
    cl_event e;
    cl_int status;
    status = clEnqueueCopyBuffer(mQueue[device],srcBuffer,dstBuffer,srcOffset,dstOffset,cb,waitList.size(),waitList.events(),&e);
    if (!MINICL_CHECK_STATUS(status)) return Event(status);
    return Event(e);
  }

  /** Return max workgroup size for kernel
      @param kernelID is the kernel ID.
      @return max workgroup size, or 0 on failure. */
  inline size_t getMaxWorkgroupSize(int device,int kernelID)
  {
    if (device < 0 || device >= (int)mDevice.size()) return 0;
    if (kernelID < 0 || kernelID >= (int)mKernel.size()) return 0;

    size_t result;
    cl_int status = clGetKernelWorkGroupInfo(mKernel[kernelID],mDevice[device],CL_KERNEL_WORK_GROUP_SIZE,sizeof(size_t),&result,0);
    if (!MINICL_CHECK_STATUS(status)) return 0;

    return result;
  }

  // Run 1D or 2D kernel on device.
  inline Event enqueueKernel(int device,int kernelID,size_t nx,size_t ny,size_t wgx,size_t wgy,const EventVector & waitList)
  {
    if (device < 0 || device >= (int)mDevice.size()) return Event(CL_INVALID_DEVICE);
    if (nx <= 0 || ny <= 0) return Event(CL_INVALID_WORK_ITEM_SIZE);
    if (wgx <= 0 || wgy <= 0) return Event(CL_INVALID_WORK_GROUP_SIZE);

    cl_int status;
    size_t nThreads[3];
    nThreads[0] = nx;
    nThreads[1] = ny;
    nThreads[2] = 1;
    size_t workGroup[3];
    workGroup[0] = wgx;
    workGroup[1] = wgy;
    workGroup[2] = 1;
    cl_event e;
    status = clEnqueueNDRangeKernel(mQueue[device],mKernel[kernelID],(ny<=(size_t)1)?1:2,0,nThreads,workGroup,waitList.size(),waitList.events(),&e);
    if (!MINICL_CHECK_STATUS(status)) return Event(status);
    return Event(e); // OK
  }

  // Clear kernel arg list
  inline void clearArgs(int kernelID) { mKernelIndex[kernelID] = 0; }

  // Push an argument in kernel arg list
  template <class T> cl_int pushArg(int kernelID,const T & x)
  {
    cl_int status = clSetKernelArg(mKernel[kernelID],mKernelIndex[kernelID]++,sizeof(T),&x);
    if (!MINICL_CHECK_STATUS(status)) return status;
    return CL_SUCCESS;
  }

  // Push a local memory argument in kernel arg list
  cl_int pushLocalArg(int kernelID,size_t sz)
  {
    cl_int status = clSetKernelArg(mKernel[kernelID],mKernelIndex[kernelID]++,sz,0);
    if (!MINICL_CHECK_STATUS(status)) return status;
    return CL_SUCCESS;
  }

private:

  // Constructor
  inline Context() : mContext(0), mProgram(0) { }

  cl_context mContext; // OpenCL context
  cl_program mProgram; // Program
  std::vector<cl_kernel> mKernel; // Kernels
  std::vector<cl_device_id> mDevice; // Devices in context
  std::vector<cl_command_queue> mQueue; // One command queue per context device
  std::vector<size_t> mMaxWorkGroupSize; // Max workgroup size for each device
  std::vector<int> mKernelIndex; // Current number of args set in each kernel

}; // class Context

/** Get platform info string
    @param platform is the query platform.
    @param param is the param to get.
    @param s receives the result.
    @retun TRUE if OK, and FALSE otherwise. */
bool getPlatformInfo(cl_platform_id platform,cl_platform_info param,std::string & s)
{
  s.clear();
  cl_int status;
  size_t sz;

  status = clGetPlatformInfo(platform,param,0,0,&sz);
  if (status != CL_SUCCESS) return false;
  s.resize(sz+8,0);
  status = clGetPlatformInfo(platform,param,sz,&(s[0]),&sz);
  if (status != CL_SUCCESS) return false;
  s.resize(sz);
  return true; // OK
}

/** Get device info string
    @param device is the query device.
    @param param is the param to get.
    @param s receives the result.
    @retun TRUE if OK, and FALSE otherwise. */
bool getDeviceInfo(cl_device_id device,cl_device_info param,std::string & s)
{
  s.clear();
  cl_int status;
  size_t sz;

  status = clGetDeviceInfo(device,param,0,0,&sz);
  if (status != CL_SUCCESS) return false;
  s.resize(sz+8,0);
  status = clGetDeviceInfo(device,param,sz,&(s[0]),&sz);
  if (status != CL_SUCCESS) return false;
  s.resize(sz);
  return true; // OK
}

/** Create an OpenCL context including all GPU devices on the first platform providing GPU devices.
    @param verbose, if TRUE display device info.
    @return a valid OpenCL context on success, and 0 otherwise. */
cl_context createGPUContext(bool verbose)
{
  const int MAX_PLATFORMS = 8;
  const int MAX_DEVICES = 16;
  cl_platform_id platform[MAX_PLATFORMS];
  cl_device_id device[MAX_DEVICES];
  cl_uint nPlatforms = 0;
  cl_uint nDevices = 0;
  cl_context context = 0;
  cl_int status = clGetPlatformIDs(MAX_PLATFORMS,platform,&nPlatforms);
  if (status < 0 || nPlatforms == 0) return 0; // No platform

  for (cl_uint p=0;p<nPlatforms;p++)
  {
    nDevices = 0;
    status = clGetDeviceIDs(platform[p],CL_DEVICE_TYPE_GPU,MAX_DEVICES,device,&nDevices);
    if (status < 0 || nDevices == 0) continue; // Failed for this platform

    // Try to create a context using all devices
    cl_context_properties props[5];
    int index = 0;
    props[index++] = CL_CONTEXT_PLATFORM;
    props[index++] = (cl_context_properties)platform[p];
    props[index++] = 0;
    context = clCreateContext(props,nDevices,device,0,0,0);
    if (context == 0) continue;

    if (verbose)
      {
	std::string s;
	fprintf(stderr,"Created context using %d devices\n",(int)nDevices);
	if (getPlatformInfo(platform[p],CL_PLATFORM_NAME,s)) fprintf(stderr,"Platform: %s / ",s.c_str());
	if (getPlatformInfo(platform[p],CL_PLATFORM_VENDOR,s)) fprintf(stderr,"%s\n",s.c_str());
	for (cl_uint d=0;d<nDevices;d++)
	  {
	    fprintf(stderr,"Device %d:\n",(int)d);
	    if (getDeviceInfo(device[d],CL_DEVICE_NAME,s)) fprintf(stderr,"- name: %s\n",s.c_str());
	    if (getDeviceInfo(device[d],CL_DRIVER_VERSION,s)) fprintf(stderr,"- driver: %s\n",s.c_str());
	    if (getDeviceInfo(device[d],CL_DEVICE_EXTENSIONS,s)) fprintf(stderr,"- extensions: %s\n",s.c_str());
	    
	  }
      }
    return context; // OK
  }

  return 0; // Failed
}

END_MINICL_NAMESPACE

#endif // #ifndef MiniCL_h
