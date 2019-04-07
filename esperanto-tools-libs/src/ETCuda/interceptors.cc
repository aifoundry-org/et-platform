#include "demangle.h"
#include <assert.h>
#include <dlfcn.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <unordered_map>

#include "BLAS/etblas.h"
#include "C-API/etrt.h"
#include "Common/et-misc.h"
#include "DNN/etdnn.h"

static bool is_interceptor_log_print() {
  static const char *s = getenv("ETT_INTERCEPTOR_LOG");
  return s && s[0] != '\0';
}

__attribute__((format(printf, 1, 2))) static int
interceptor_log(const char *fmt, ...) {
  if (is_interceptor_log_print()) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return ret;
  }
  return 0;
}

static bool use_cuda() {
  static const char *s = getenv("ETT_CUDA");
  return s && s[0] != '\0';
}

template <typename Ret, typename... Args>
Ret InterceptFunction(const char *dlsym_name, Ret (*internal_fun_p)(Args...),
                      Args... args) {
  typedef Ret (*fun_type_t)(Args...);

  if (use_cuda()) {
    fun_type_t dlsym_fun_p = (fun_type_t)dlsym(RTLD_NEXT, dlsym_name);

    if (!dlsym_fun_p) {
      fprintf(stderr, "ERROR: Original '%s' not found\n", dlsym_name);
      abort();
    }

    return dlsym_fun_p(args...);
  } else {
    return internal_fun_p(args...);
  }
}

#define INTERCEPTOR_STUB(sym_name)                                             \
  EXAPI void sym_name() {                                                      \
    fprintf(stderr, "ERROR: Unimplemented symbol: %s\n", #sym_name);           \
    abort();                                                                   \
  }

/*
 * hostFun -> deviceFun(=deviceName)
 * Note: actually, deviceName is the mangled name of hostFun.
 */
static std::unordered_map<const void *, std::string> gRegisteredFunctions;
/*
 * Note: __cudaRegisterFunction is called during DSO linking (before our global
 * variables constructors), so in __cudaRegisterFunction we may only store data
 * to statically initialized data types (POD).
 */
static const int kRegisteredFunctionsMax = 1024;
static const void *gRegisteredFunctionsPtr[kRegisteredFunctionsMax];
static const char *gRegisteredFunctionsName[kRegisteredFunctionsMax];
static int gRegisteredFunctionsNum = 0;

static const std::string &find_registered_function_name(const void *hostFun) {
  static std::once_flag flag;
  std::call_once(flag, []() {
    for (int i = 0; i < gRegisteredFunctionsNum; i++) {
      gRegisteredFunctions.insert(
          {gRegisteredFunctionsPtr[i], gRegisteredFunctionsName[i]});
    }
  });

  assert(gRegisteredFunctions.find(hostFun) != gRegisteredFunctions.end());
  return gRegisteredFunctions[hostFun];
}

////////////////////////////////////////////////////////////////////////////////////
// cudart: /usr/local/cuda-9.1/targets/x86_64-linux/include/crt/host_runtime.h
////////////////////////////////////////////////////////////////////////////////////

EXAPI void **__cudaRegisterFatBinary(void *fatCubin) {
  void **fatCubinHandle = InterceptFunction<void **>(
      "__cudaRegisterFatBinary", __etrtRegisterFatBinary, fatCubin);

  //    interceptor_log("__cudaRegisterFatBinary fcbhandle:%p fcb:%p\n",
  //    fatCubinHandle, fatCubin);
  return fatCubinHandle;
}

EXAPI void __cudaUnregisterFatBinary(void **fatCubinHandle) {
  InterceptFunction<void>("__cudaUnregisterFatBinary",
                          __etrtUnregisterFatBinary, fatCubinHandle);

  //    interceptor_log("__cudaUnregisterFatBinary fcbhandle:%p\n",
  //    fatCubinHandle);
}

EXAPI void __cudaRegisterFunction(void **fatCubinHandle, const char *hostFun,
                                  char *deviceFun, const char *deviceName,
                                  int thread_limit, uint3 *tid, uint3 *bid,
                                  dim3 *bDim, dim3 *gDim, int *wSize) {
  InterceptFunction<void>("__cudaRegisterFunction", __etrtRegisterFunction,
                          fatCubinHandle, hostFun, deviceFun, deviceName,
                          thread_limit, tid, bid, bDim, gDim, wSize);

  assert(deviceFun == deviceName);
  //    interceptor_log("__cudaRegisterFunction fcbhandle:%p %p %s %d %p %p %p
  //    %p %p [%s]\n",
  //                    fatCubinHandle, hostFun, deviceName, thread_limit, tid,
  //                    bid, bDim, gDim, wSize, demangle(deviceName).c_str());

  if (gRegisteredFunctionsNum >= kRegisteredFunctionsMax) {
    abort();
  }
  gRegisteredFunctionsPtr[gRegisteredFunctionsNum] = hostFun;
  gRegisteredFunctionsName[gRegisteredFunctionsNum] = deviceName;
  gRegisteredFunctionsNum++;
}

EXAPI void __cudaRegisterVar(void **fatCubinHandle, char *hostVar,
                             char *deviceAddress, const char *deviceName,
                             int ext, size_t size, int constant, int global) {
  InterceptFunction<void>("__cudaRegisterVar", __etrtRegisterVar,
                          fatCubinHandle, hostVar, deviceAddress, deviceName,
                          ext, size, constant, global);

  assert(deviceAddress == deviceName);
  //    interceptor_log("__cudaRegisterVar fcbhandle:%p %p %s %d %lu %d %d\n",
  //                    fatCubinHandle, hostVar, deviceName, ext, size,
  //                    constant, global);
}

////////////////////////////////////////////////////////////////////////////////////
// cudart: /usr/local/cuda-9.1/targets/x86_64-linux/include/cuda_runtime_api.h
////////////////////////////////////////////////////////////////////////////////////

EXAPI const char *cudaGetErrorString(etrtError_t error) {
  const char *res = InterceptFunction<const char *>("cudaGetErrorString",
                                                    etrtGetErrorString, error);
  interceptor_log("cudaGetErrorString err:%d res:%s\n", error, res);
  return res;
}

EXAPI etrtError_t cudaGetDeviceCount(int *count) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaGetDeviceCount",
                                                   etrtGetDeviceCount, count);
  interceptor_log("cudaGetDeviceCount err:%d count:%d\n", res, *count);
  return res;
}

EXAPI etrtError_t cudaGetDeviceProperties(struct etrtDeviceProp *prop,
                                          int device) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaGetDeviceProperties", etrtGetDeviceProperties, prop, device);
  interceptor_log("cudaGetDeviceProperties err:%d device:%d major:%d minor:%d "
                  "name:%s totalGlobalMem:%lu maxThreadsPerBlock:%d\n",
                  res, device, prop->major, prop->minor, prop->name,
                  prop->totalGlobalMem, prop->maxThreadsPerBlock);
  return res;
}

/**
 * \brief Returns which device is currently being used
 *
 * Returns in \p *device the current device for the calling host thread.
 *
 * \param device - Returns the device on which the active host thread
 * executes the device code.
 *
 * \return
 * ::etrtSuccess
 * \notefnerr
 *
 * \sa ::cudaGetDeviceCount, ::cudaSetDevice, ::cudaGetDeviceProperties,
 * ::cudaChooseDevice,
 * ::cuCtxGetCurrent
 */
EXAPI etrtError_t cudaGetDevice(int *device) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaGetDevice", etrtGetDevice, device);
  // interceptor_log("cudaGetDevice err:%d device:%d\n", res, *device);
  return res;
}

/**
 * \brief Set device to be used for GPU executions
 *
 * Sets \p device as the current device for the calling host thread.
 * Valid device id's are 0 to (::cudaGetDeviceCount() - 1).
 *
 * Any device memory subsequently allocated from this host thread
 * using ::cudaMalloc(), ::cudaMallocPitch() or ::cudaMallocArray()
 * will be physically resident on \p device.  Any host memory allocated
 * from this host thread using ::cudaMallocHost() or ::cudaHostAlloc()
 * or ::cudaHostRegister() will have its lifetime associated  with
 * \p device.  Any streams or events created from this host thread will
 * be associated with \p device.  Any kernels launched from this host
 * thread using the <<<>>> operator or ::cudaLaunchKernel() will be executed
 * on \p device.
 *
 * This call may be made from any host thread, to any device, and at
 * any time.  This function will do no synchronization with the previous
 * or new device, and should be considered a very low overhead call.
 *
 * \param device - Device on which the active host thread should execute the
 * device code.
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInvalidDevice,
 * ::cudaErrorDeviceAlreadyInUse
 * \notefnerr
 *
 * \sa ::cudaGetDeviceCount, ::cudaGetDevice, ::cudaGetDeviceProperties,
 * ::cudaChooseDevice,
 * ::cuCtxSetCurrent
 */
EXAPI etrtError_t cudaSetDevice(int device) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaSetDevice", etrtSetDevice, device);
  // interceptor_log("cudaSetDevice err:%d device:%d\n", res, device);
  return res;
}

EXAPI etrtError_t cudaMallocHost(void **ptr, size_t size) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaMallocHost",
                                                   etrtMallocHost, ptr, size);
  interceptor_log("cudaMallocHost err:%d ptr:%p size:%lu\n", res, *ptr, size);
  return res;
}

EXAPI etrtError_t cudaFreeHost(void *ptr) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaFreeHost", etrtFreeHost, ptr);
  interceptor_log("cudaFreeHost err:%d ptr:%p\n", res, ptr);
  return res;
}

/**
 * \brief Allocate memory on the device
 *
 * Allocates \p size bytes of linear memory on the device and returns in
 * \p *devPtr a pointer to the allocated memory. The allocated memory is
 * suitably aligned for any kind of variable. The memory is not cleared.
 * ::cudaMalloc() returns ::etrtErrorMemoryAllocation in case of failure.
 *
 * The device version of ::cudaFree cannot be used with a \p *devPtr
 * allocated using the host API, and vice versa.
 *
 * \param devPtr - Pointer to allocated device memory
 * \param size   - Requested allocation size in bytes
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInvalidValue,
 * ::etrtErrorMemoryAllocation
 *
 * \sa ::cudaMallocPitch, ::cudaFree, ::cudaMallocArray, ::cudaFreeArray,
 * ::cudaMalloc3D, ::cudaMalloc3DArray,
 * \ref ::cudaMallocHost(void**, size_t) "cudaMallocHost (C API)",
 * ::cudaFreeHost, ::cudaHostAlloc,
 * ::cuMemAlloc
 */
EXAPI etrtError_t cudaMalloc(void **devPtr, size_t size) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaMalloc", etrtMalloc, devPtr, size);
  interceptor_log("cudaMalloc err:%d devPtr:%p size:%lu\n", res, *devPtr, size);
  return res;
}

EXAPI etrtError_t cudaFree(void *devPtr) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaFree", etrtFree, devPtr);
  interceptor_log("cudaFree err:%d devPtr:%p\n", res, devPtr);
  return res;
}

EXAPI etrtError_t cudaPointerGetAttributes(
    struct etrtPointerAttributes *attributes, const void *ptr) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaPointerGetAttributes", etrtPointerGetAttributes, attributes, ptr);
  interceptor_log("cudaPointerGetAttributes err:%d ptr:%p memType:%d, "
                  "device:%d, devPtr:%p hostPtr:%p isManaged:%d\n",
                  res, ptr, attributes->memoryType, attributes->device,
                  attributes->devicePointer, attributes->hostPointer,
                  attributes->isManaged);
  return res;
}

EXAPI etrtError_t cudaStreamCreate(etrtStream_t *pStream) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaStreamCreate",
                                                   etrtStreamCreate, pStream);
  interceptor_log("cudaStreamCreate err:%d stream:%p\n", res, *pStream);
  return res;
}

/**
 * \brief Create an asynchronous stream
 *
 * Creates a new asynchronous stream.  The \p flags argument determines the
 * behaviors of the stream.  Valid values for \p flags are
 * - ::etrtStreamDefault: Default stream creation flag.
 * - ::etrtStreamNonBlocking: Specifies that work running in the created
 *   stream may run concurrently with work in stream 0 (the NULL stream), and
 * that the created stream should perform no implicit synchronization with
 * stream 0.
 *
 * \param pStream - Pointer to new stream identifier
 * \param flags   - Parameters for stream creation
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInvalidValue
 * \notefnerr
 *
 * \sa ::cudaStreamCreate,
 * ::cudaStreamCreateWithPriority,
 * ::cudaStreamGetFlags,
 * ::cudaStreamQuery,
 * ::cudaStreamSynchronize,
 * ::cudaStreamWaitEvent,
 * ::cudaStreamAddCallback,
 * ::cudaStreamDestroy,
 * ::cuStreamCreate
 */
EXAPI etrtError_t cudaStreamCreateWithFlags(etrtStream_t *pStream,
                                            unsigned int flags) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaStreamCreateWithFlags", etrtStreamCreateWithFlags, pStream, flags);
  interceptor_log("cudaStreamCreateWithFlags err:%d stream:%p flags:0x%x\n",
                  res, *pStream, flags);
  return res;
}

EXAPI etrtError_t cudaStreamDestroy(etrtStream_t stream) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaStreamDestroy",
                                                   etrtStreamDestroy, stream);
  interceptor_log("cudaStreamDestroy err:%d stream:%p\n", res, stream);
  return res;
}

EXAPI etrtError_t cudaMemcpy(void *dst, const void *src, size_t count,
                             enum etrtMemcpyKind kind) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaMemcpy", etrtMemcpy,
                                                   dst, src, count, kind);
  interceptor_log("cudaMemcpy err:%d dst:%p src:%p count:%lu kind:%d\n", res,
                  dst, src, count, kind);
  return res;
}

EXAPI etrtError_t cudaMemcpyAsync(void *dst, const void *src, size_t count,
                                  enum etrtMemcpyKind kind,
                                  etrtStream_t stream /*__dv(0)*/) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaMemcpyAsync", etrtMemcpyAsync, dst, src, count, kind, stream);
  interceptor_log(
      "cudaMemcpyAsync err:%d dst:%p src:%p count:%lu kind:%d, stream:%p\n",
      res, dst, src, count, kind, stream);
  return res;
}

EXAPI etrtError_t cudaMemset(void *devPtr, int value, size_t count) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaMemset", etrtMemset,
                                                   devPtr, value, count);
  interceptor_log("cudaMemset err:%d devPtr:%p value:%d count:%lu \n", res,
                  devPtr, value, count);
  return res;
}

/**
 * \brief Waits for stream tasks to complete
 *
 * Blocks until \p stream has completed all operations. If the
 * ::cudaDeviceScheduleBlockingSync flag was set for this device,
 * the host thread will block until the stream is finished with
 * all of its tasks.
 *
 * \param stream - Stream identifier
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInvalidResourceHandle
 * \note_null_stream
 * \notefnerr
 *
 * \sa ::cudaStreamCreate, ::cudaStreamCreateWithFlags, ::cudaStreamQuery,
 * ::cudaStreamWaitEvent, ::cudaStreamAddCallback, ::cudaStreamDestroy,
 * ::cuStreamSynchronize
 */
EXAPI etrtError_t cudaStreamSynchronize(etrtStream_t stream) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaStreamSynchronize", etrtStreamSynchronize, stream);
  interceptor_log("cudaStreamSynchronize err:%d stream:%p\n", res, stream);
  return res;
}

EXAPI etrtError_t cudaGetLastError(void) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaGetLastError", etrtGetLastError);
  interceptor_log("cudaGetLastError err:%d\n", res);
  return res;
}

EXAPI etrtError_t cudaEventCreate(etrtEvent_t *event) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaEventCreate", etrtEventCreate, event);
  interceptor_log("cudaEventCreate err:%d event:%p\n", res, *event);
  return res;
}

/**
 * \brief Creates an event object with the specified flags
 *
 * Creates an event object for the current device with the specified flags.
 * Valid flags include:
 * - ::cudaEventDefault: Default event creation flag.
 * - ::cudaEventBlockingSync: Specifies that event should use blocking
 *   synchronization. A host thread that uses ::cudaEventSynchronize() to wait
 *   on an event created with this flag will block until the event actually
 *   completes.
 * - ::cudaEventDisableTiming: Specifies that the created event does not need
 *   to record timing data.  Events created with this flag specified and
 *   the ::cudaEventBlockingSync flag not specified will provide the best
 *   performance when used with ::cudaStreamWaitEvent() and ::cudaEventQuery().
 * - ::cudaEventInterprocess: Specifies that the created event may be used as an
 *   interprocess event by ::cudaIpcGetEventHandle(). ::cudaEventInterprocess
 * must be specified along with ::cudaEventDisableTiming.
 *
 * \param event - Newly created event
 * \param flags - Flags for new event
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInitializationError,
 * ::etrtErrorInvalidValue,
 * ::etrtErrorLaunchFailure,
 * ::etrtErrorMemoryAllocation
 * \notefnerr
 *
 * \sa \ref ::cudaEventCreate(cudaEvent_t*) "cudaEventCreate (C API)",
 * ::cudaEventSynchronize, ::cudaEventDestroy, ::cudaEventElapsedTime,
 * ::cudaStreamWaitEvent,
 * ::cuEventCreate
 */
EXAPI etrtError_t cudaEventCreateWithFlags(etrtEvent_t *event,
                                           unsigned int flags) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaEventCreateWithFlags", etrtEventCreateWithFlags, event, flags);
  interceptor_log("cudaEventCreateWithFlags err:%d event:%p flags:0x%x\n", res,
                  *event, flags);
  return res;
}

EXAPI etrtError_t cudaEventQuery(etrtEvent_t event) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaEventQuery", etrtEventQuery, event);
  interceptor_log("cudaEventQuery err:%d event:%p\n", res, event);
  return res;
}

EXAPI etrtError_t cudaEventRecord(etrtEvent_t event, etrtStream_t stream) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaEventRecord", etrtEventRecord, event, stream);
  interceptor_log("cudaEventRecord err:%d event:%p stream:%p\n", res, event,
                  stream);
  return res;
}

EXAPI etrtError_t cudaStreamWaitEvent(etrtStream_t stream, etrtEvent_t event,
                                      unsigned int flags) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaStreamWaitEvent", etrtStreamWaitEvent, stream, event, flags);
  interceptor_log("cudaStreamWaitEvent err:%d stream:%p event:%p flags:0x%x\n",
                  res, stream, event, flags);
  return res;
}

EXAPI etrtError_t cudaEventSynchronize(etrtEvent_t event) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaEventSynchronize",
                                                   etrtEventSynchronize, event);
  interceptor_log("cudaEventSynchronize err:%d event:%p\n", res, event);
  return res;
}

EXAPI etrtError_t cudaEventElapsedTime(float *ms, etrtEvent_t start,
                                       etrtEvent_t end) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaEventElapsedTime", etrtEventElapsedTime, ms, start, end);
  interceptor_log("cudaEventElapsedTime err:%d ms:%f start:%p end:%p\n", res,
                  *ms, start, end);
  return res;
}

EXAPI etrtError_t cudaEventDestroy(etrtEvent_t event) {
  etrtError_t res = InterceptFunction<etrtError_t>("cudaEventDestroy",
                                                   etrtEventDestroy, event);
  interceptor_log("cudaEventDestroy err:%d event:%p\n", res, event);
  return res;
}

/**
 * \brief Configure a device-launch
 *
 * \deprecated This function is deprecated as of CUDA 7.0
 *
 * Specifies the grid and block dimensions for the device call to be executed
 * similar to the execution configuration syntax. ::cudaConfigureCall() is
 * stack based. Each call pushes data on top of an execution stack. This data
 * contains the dimension for the grid and thread blocks, together with any
 * arguments for the call.
 *
 * \param gridDim   - Grid dimensions
 * \param blockDim  - Block dimensions
 * \param sharedMem - Shared memory
 * \param stream    - Stream identifier
 *
 * \return
 * ::etrtSuccess,
 * ::etrtErrorInvalidConfiguration
 * \note_null_stream
 * \notefnerr
 *
 * \sa
 * \ref ::cudaLaunchKernel(const void *func, dim3 gridDim, dim3 blockDim, void
 * **args, size_t sharedMem, cudaStream_t stream) "cudaLaunchKernel (C API)",
 * \ref ::cudaFuncSetCacheConfig(const void*, enum cudaFuncCache)
 * "cudaFuncSetCacheConfig (C API)", \ref ::cudaFuncGetAttributes(struct
 * cudaFuncAttributes*, const void*) "cudaFuncGetAttributes (C API)", \ref
 * ::cudaLaunch(const void*) "cudaLaunch (C API)",
 * ::cudaSetDoubleForDevice,
 * ::cudaSetDoubleForHost,
 * \ref ::cudaSetupArgument(const void*, size_t, size_t) "cudaSetupArgument (C
 * API)",
 */
EXAPI etrtError_t cudaConfigureCall(dim3 gridDim, dim3 blockDim,
                                    size_t sharedMem /*__dv(0)*/,
                                    etrtStream_t stream /*__dv(0)*/) {
  etrtError_t res =
      InterceptFunction<etrtError_t>("cudaConfigureCall", etrtConfigureCall,
                                     gridDim, blockDim, sharedMem, stream);
  interceptor_log("cudaConfigureCall err:%d gridDim:(%d,%d,%d) "
                  "blockDim:(%d,%d,%d) sharedMem:%lu stream:%p\n",
                  res, gridDim.x, gridDim.y, gridDim.z, blockDim.x, blockDim.y,
                  blockDim.z, sharedMem, stream);
  return res;
}

EXAPI etrtError_t cudaSetupArgument(const void *arg, size_t size,
                                    size_t offset) {
  etrtError_t res = InterceptFunction<etrtError_t>(
      "cudaSetupArgument", etrtSetupArgument, arg, size, offset);

  char arg_val_str[256] = "";
  switch (size) {
  case 1:
    sprintf(arg_val_str, "arg_val:%d(0x%x)", *(uint8_t *)arg, *(uint8_t *)arg);
    break;
  case 2:
    sprintf(arg_val_str, "arg_val:%d(0x%x)", *(uint16_t *)arg,
            *(uint16_t *)arg);
    break;
  case 4:
    sprintf(arg_val_str, "arg_val:%d(0x%x)", *(int *)arg, *(int *)arg);
    break;
  case 8:
    sprintf(arg_val_str, "arg_val:%lld(0x%llx)", *(long long *)arg,
            *(long long *)arg);
    break;
  }

  interceptor_log("cudaSetupArgument err:%d arg:%p size:%lu offset:%lu, [%s]\n",
                  res, arg, size, offset, arg_val_str);

  return res;
}

EXAPI etrtError_t cudaLaunch(const void *func) {
  typedef etrtError_t (*cudaLaunch_t)(const void *func);

  cudaLaunch_t cudaLaunch_p = (cudaLaunch_t)dlsym(RTLD_NEXT, "cudaLaunch");

  etrtError_t err =
      use_cuda()
          ? cudaLaunch_p(func)
          : etrtLaunch(func, find_registered_function_name(func).c_str());

  const std::string &name = find_registered_function_name(func);
  interceptor_log("cudaLaunch err:%d hostFun:%p [-> %s [%s]]\n", err, func,
                  name.c_str(), demangle(name).c_str());

  return err;
}

INTERCEPTOR_STUB(cudaMemcpy2DAsync)
INTERCEPTOR_STUB(cudaDeviceCanAccessPeer)
INTERCEPTOR_STUB(cudaDeviceEnablePeerAccess)
INTERCEPTOR_STUB(cudaHostRegister)
INTERCEPTOR_STUB(cudaHostUnregister)
INTERCEPTOR_STUB(cudaStreamQuery)

////////////////////////////////////////////////////////////////////////////////////
// cublas: /usr/local/cuda/include/cublas_api.h
////////////////////////////////////////////////////////////////////////////////////

EXAPI etblasStatus_t cublasCreate_v2(etblasHandle_t *handle) {
  etblasStatus_t res = InterceptFunction<etblasStatus_t>(
      "cublasCreate_v2", etblasCreate_v2, handle);
  interceptor_log("cublasCreate_v2 status:%d handle:%p\n", res, *handle);
  return res;
}

EXAPI etblasStatus_t cublasDestroy_v2(etblasHandle_t handle) {
  etblasStatus_t res = InterceptFunction<etblasStatus_t>(
      "cublasDestroy_v2", etblasDestroy_v2, handle);
  interceptor_log("cublasDestroy_v2 status:%d handle:%p\n", res, handle);
  return res;
}

EXAPI etblasStatus_t cublasSetPointerMode_v2(etblasHandle_t handle,
                                             etblasPointerMode_t mode) {
  etblasStatus_t res = InterceptFunction<etblasStatus_t>(
      "cublasSetPointerMode_v2", etblasSetPointerMode_v2, handle, mode);
  interceptor_log("cublasSetPointerMode_v2 status:%d handle:%p mode:%d\n", res,
                  handle, mode);
  return res;
}

EXAPI etblasStatus_t cublasSetStream_v2(etblasHandle_t handle,
                                        etrtStream_t streamId) {
  etblasStatus_t res = InterceptFunction<etblasStatus_t>(
      "cublasSetStream_v2", etblasSetStream_v2, handle, streamId);
  interceptor_log("cublasSetStream_v2 status:%d handle:%p stream:%p\n", res,
                  handle, streamId);
  return res;
}

EXAPI etblasStatus_t cublasSgemm_v2(
    etblasHandle_t handle, etblasOperation_t transa, etblasOperation_t transb,
    int m, int n, int k, const float *alpha, /* host or device pointer */
    const float *A, int lda, const float *B, int ldb,
    const float *beta, /* host or device pointer */
    float *C, int ldc) {
  etblasStatus_t res = InterceptFunction<etblasStatus_t>(
      "cublasSgemm_v2", etblasSgemm_v2, handle, transa, transb, m, n, k, alpha,
      A, lda, B, ldb, beta, C, ldc);
  interceptor_log(
      "cublasSgemm_v2 status:%d handle:%p transa:%d transb:%d m:%d n:%d k:%d, "
      "alpha:%p A:%p lda:%d B:%p ldb:%d beta:%p C:%p ldc:%d\n",
      res, handle, transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C,
      ldc);
  return res;
}

INTERCEPTOR_STUB(cublasSetMathMode)
INTERCEPTOR_STUB(cublasHgemm)
INTERCEPTOR_STUB(cublasGemmEx)
INTERCEPTOR_STUB(cublasSgemmEx)
INTERCEPTOR_STUB(cublasAxpyEx)
INTERCEPTOR_STUB(cublasDotEx)
INTERCEPTOR_STUB(cublasSaxpy_v2)
INTERCEPTOR_STUB(cublasDaxpy_v2)
INTERCEPTOR_STUB(cublasSgemv_v2)
INTERCEPTOR_STUB(cublasSdot_v2)

////////////////////////////////////////////////////////////////////////////////////
// cudnn: /usr/include/cudnn.h
////////////////////////////////////////////////////////////////////////////////////

EXAPI size_t cudnnGetVersion(void) {
  size_t res = InterceptFunction<size_t>("cudnnGetVersion", etdnnGetVersion);
  interceptor_log("cudnnGetVersion res:%lu\n", res);
  return res;
}

EXAPI etdnnStatus_t cudnnCreate(etdnnHandle_t *handle) {
  etdnnStatus_t res =
      InterceptFunction<etdnnStatus_t>("cudnnCreate", etdnnCreate, handle);
  interceptor_log("cudnnCreate status:%d handle:%p\n", res, *handle);
  return res;
}

EXAPI etdnnStatus_t cudnnDestroy(etdnnHandle_t handle) {
  etdnnStatus_t res =
      InterceptFunction<etdnnStatus_t>("cudnnDestroy", etdnnDestroy, handle);
  interceptor_log("cudnnDestroy status:%d handle:%p\n", res, handle);
  return res;
}

EXAPI etdnnStatus_t cudnnSetStream(etdnnHandle_t handle,
                                   etrtStream_t streamId) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetStream", etdnnSetStream, handle, streamId);
  interceptor_log("cudnnSetStream status:%d handle:%p stream:%p\n", res, handle,
                  streamId);
  return res;
}

EXAPI etdnnStatus_t cudnnGetStream(etdnnHandle_t handle,
                                   etrtStream_t *streamId) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnGetStream", etdnnGetStream, handle, streamId);
  interceptor_log("cudnnGetStream status:%d handle:%p stream:%p\n", res, handle,
                  *streamId);
  return res;
}

/**
    This function creates a generic tensor descriptor object by allocating the
   memory needed to hold its opaque structure. The data is initialized to be all
   zero.

    Parameters

    tensorDesc
    Input. Pointer to pointer where the address to the allocated tensor
   descriptor object should be stored*

    Returns

    CUDNN_STATUS_BAD_PARAM
    Invalid input argument.

    CUDNN_STATUS_ALLOC_FAILED
    The resources could not be allocated.

    CUDNN_STATUS_SUCCESS
    The object was created successfully.
 */
EXAPI etdnnStatus_t
cudnnCreateTensorDescriptor(etdnnTensorDescriptor_t *tensorDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnCreateTensorDescriptor", etdnnCreateTensorDescriptor, tensorDesc);
  interceptor_log("cudnnCreateTensorDescriptor status:%d tensorDesc:%p\n", res,
                  *tensorDesc);
  return res;
}

/*
    This function destroys a previously created tensor descriptor object. When
   the input pointer is NULL, this function performs no destroy operation.

    Parameters

    tensorDesc
    Input. Pointer to the tensor descriptor object to be destroyed.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was destroyed successfully.
*/
EXAPI etdnnStatus_t
cudnnDestroyTensorDescriptor(etdnnTensorDescriptor_t tensorDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDestroyTensorDescriptor", etdnnDestroyTensorDescriptor, tensorDesc);
  interceptor_log("cudnnDestroyTensorDescriptor status:%d tensorDesc:%p\n", res,
                  tensorDesc);
  return res;
}

/*
    This function initializes a previously created generic Tensor descriptor
   object into a 4D tensor, similarly to cudnnSetTensor4dDescriptor but with the
   strides explicitly passed as parameters. This can be used to lay out the 4D
   tensor in any order or simply to define gaps between dimensions.

    Note: At present, some cuDNN routines have limited support for strides;
   Those routines will return CUDNN_STATUS_NOT_SUPPORTED if a Tensor4D object
   with an unsupported stride is used. cudnnTransformTensor can be used to
   convert the data to a supported layout. Note: The total size of a tensor
   including the potential padding between dimensions is limited to 2
   Giga-elements of type datatype. Parameters

    tensorDesc
    Input/Output. Handle to a previously created tensor descriptor.

    datatype
    Input. Data type.

    n
    Input. Number of images.

    c
    Input. Number of feature maps per image.

    h
    Input. Height of each feature map.

    w
    Input. Width of each feature map.

    nStride
    Input. Stride between two consecutive images.

    cStride
    Input. Stride between two consecutive feature maps.

    hStride
    Input. Stride between two consecutive rows.

    wStride
    Input. Stride between two consecutive columns.

    The possible error values returned by this function and their meanings are
   listed below.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was set successfully.

    CUDNN_STATUS_BAD_PARAM
    At least one of the parameters n,c,h,w or nStride,cStride,hStride,wStride is
   negative or dataType has an invalid enumerant value.

    CUDNN_STATUS_NOT_SUPPORTED
    The total size of the tensor descriptor exceeds the maximim limit of 2
   Giga-elements.
*/
EXAPI etdnnStatus_t cudnnSetTensor4dDescriptorEx(
    etdnnTensorDescriptor_t tensorDesc, etdnnDataType_t dataType, int n, int c,
    int h, int w, int nStride, int cStride, int hStride, int wStride) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetTensor4dDescriptorEx", etdnnSetTensor4dDescriptorEx, tensorDesc,
      dataType, n, c, h, w, nStride, cStride, hStride, wStride);
  interceptor_log(
      "cudnnSetTensor4dDescriptor status:%d tensorDesc:%p dataType:%d n:%d "
      "c:%d h:%d w:%d nStride:%d cStride:%d hStride:%d wStride:%d\n",
      res, tensorDesc, dataType, n, c, h, w, nStride, cStride, hStride,
      wStride);
  return res;
}

/*
    This function initializes a previously created generic Tensor descriptor
   object into a 4D tensor. The strides of the four dimensions are inferred from
   the format parameter and set in such a way that the data is contiguous in
   memory with no padding between dimensions.

    Note: The total size of a tensor including the potential padding between
   dimensions is limited to 2 Giga-elements of type datatype. Parameters

    tensorDesc
    Input/Output. Handle to a previously created tensor descriptor.

    format
    Input. Type of format.

    datatype
    Input. Data type.

    n
    Input. Number of images.

    c
    Input. Number of feature maps per image.

    h
    Input. Height of each feature map.

    w
    Input. Width of each feature map.

    The possible error values returned by this function and their meanings are
   listed below.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was set successfully.

    CUDNN_STATUS_BAD_PARAM
    At least one of the parameters n,c,h,w was negative or format has an invalid
   enumerant value or dataType has an invalid enumerant value.

    CUDNN_STATUS_NOT_SUPPORTED
    The total size of the tensor descriptor exceeds the maximim limit of 2
   Giga-elements.
*/
EXAPI etdnnStatus_t cudnnSetTensor4dDescriptor(
    etdnnTensorDescriptor_t tensorDesc, etdnnTensorFormat_t format,
    etdnnDataType_t dataType, int n, int c, int h, int w) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetTensor4dDescriptor", etdnnSetTensor4dDescriptor, tensorDesc,
      format, dataType, n, c, h, w);
  interceptor_log("cudnnSetTensor4dDescriptor status:%d tensorDesc:%p "
                  "format:%d dataType:%d n:%d c:%d h:%d w:%d\n",
                  res, tensorDesc, format, dataType, n, c, h, w);
  return res;
}

/*
    This function creates a filter descriptor object by allocating the memory
   needed to hold its opaque structure,

    Returns

    CUDNN_STATUS_SUCCESS
    The object was created successfully.

    CUDNN_STATUS_ALLOC_FAILED
    The resources could not be allocated.
*/
EXAPI etdnnStatus_t
cudnnCreateFilterDescriptor(etdnnFilterDescriptor_t *filterDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnCreateFilterDescriptor", etdnnCreateFilterDescriptor, filterDesc);
  interceptor_log("cudnnCreateFilterDescriptor status:%d filterDesc:%p\n", res,
                  *filterDesc);
  return res;
}

/*
    This function destroys a previously created Tensor4D descriptor object.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was destroyed successfully.
*/
EXAPI etdnnStatus_t
cudnnDestroyFilterDescriptor(etdnnFilterDescriptor_t filterDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDestroyFilterDescriptor", etdnnDestroyFilterDescriptor, filterDesc);
  interceptor_log("cudnnDestroyFilterDescriptor status:%d tensorDesc:%p\n", res,
                  filterDesc);
  return res;
}

/*
    This function initializes a previously created filter descriptor object into
   a 4D filter. Filters layout must be contiguous in memory.

    Tensor format CUDNN_TENSOR_NHWC has limited support in
   cudnnConvolutionForward, cudnnConvolutionBackwardData and
   cudnnConvolutionBackwardFilter; please refer to each function's documentation
   for more information.

    Parameters

    filterDesc
    Input/Output. Handle to a previously created filter descriptor.

    datatype
    Input. Data type.

    format
    Input. Type of format.

    k
    Input. Number of output feature maps.

    c
    Input. Number of input feature maps.

    h
    Input. Height of each filter.

    w
    Input. Width of each filter.

    The possible error values returned by this function and their meanings are
   listed below.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was set successfully.

    CUDNN_STATUS_BAD_PARAM
    At least one of the parameters k,c,h,w is negative or dataType or format has
   an invalid enumerant value.
 */
EXAPI etdnnStatus_t cudnnSetFilter4dDescriptor(
    etdnnFilterDescriptor_t filterDesc, etdnnDataType_t dataType,
    etdnnTensorFormat_t format, int k, int c, int h, int w) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetFilter4dDescriptor", etdnnSetFilter4dDescriptor, filterDesc,
      dataType, format, k, c, h, w);
  interceptor_log("cudnnSetFilter4dDescriptor status:%d filterDesc:%p "
                  "dataType:%d format:%d k:%d c:%d h:%d w:%d\n",
                  res, filterDesc, dataType, format, k, c, h, w);
  return res;
}

/*
    This function creates a convolution descriptor object by allocating the
   memory needed to hold its opaque structure,

    Returns

    CUDNN_STATUS_SUCCESS
    The object was created successfully.

    CUDNN_STATUS_ALLOC_FAILED
    The resources could not be allocated.
 */
EXAPI etdnnStatus_t
cudnnCreateConvolutionDescriptor(etdnnConvolutionDescriptor_t *convDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnCreateConvolutionDescriptor", etdnnCreateConvolutionDescriptor,
      convDesc);
  interceptor_log("cudnnCreateConvolutionDescriptor status:%d convDesc:%p\n",
                  res, *convDesc);
  return res;
}

/*
    This function destroys a previously created convolution descriptor object.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was destroyed successfully.
*/
EXAPI etdnnStatus_t
cudnnDestroyConvolutionDescriptor(etdnnConvolutionDescriptor_t convDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDestroyConvolutionDescriptor", etdnnDestroyConvolutionDescriptor,
      convDesc);
  interceptor_log("cudnnDestroyConvolutionDescriptor status:%d convDesc:%p\n",
                  res, convDesc);
  return res;
}

/*
    This function initializes a previously created convolution descriptor object
   into a 2D correlation. This function assumes that the tensor and filter
   descriptors corresponds to the formard convolution path and checks if their
   settings are valid. That same convolution descriptor can be reused in the
   backward path provided it corresponds to the same layer.

    Parameters

    convDesc
    Input/Output. Handle to a previously created convolution descriptor.

    pad_h
    Input. zero-padding height: number of rows of zeros implicitly concatenated
   onto the top and onto the bottom of input images.

    pad_w
    Input. zero-padding width: number of columns of zeros implicitly
   concatenated onto the left and onto the right of input images.

    u
    Input. Vertical filter stride.

    v
    Input. Horizontal filter stride.

    dilation_h
    Input. Filter height dilation.

    dilation_w
    Input. Filter width dilation.

    mode
    Input. Selects between CUDNN_CONVOLUTION and CUDNN_CROSS_CORRELATION.

    computeType
    Input. compute precision.

    The possible error values returned by this function and their meanings are
   listed below.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was set successfully.

    CUDNN_STATUS_BAD_PARAM
    At least one of the following conditions are met:

    The descriptor convDesc is nil.
    One of the parameters pad_h,pad_w is strictly negative.
    One of the parameters u,v is negative or zero.
    One of the parameters dilation_h,dilation_w is negative or zero.
    The parameter mode has an invalid enumerant value.
 */
EXAPI etdnnStatus_t cudnnSetConvolution2dDescriptor(
    etdnnConvolutionDescriptor_t convDesc, int pad_h, int pad_w, int u, int v,
    int dilation_h, int dilation_w, etdnnConvolutionMode_t mode,
    etdnnDataType_t computeType) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetConvolution2dDescriptor", etdnnSetConvolution2dDescriptor,
      convDesc, pad_h, pad_w, u, v, dilation_h, dilation_w, mode, computeType);
  interceptor_log(
      "cudnnSetConvolution2dDescriptor status:%d convDesc:%p pad_h:%d pad_w:%d "
      "u:%d v:%d dilation_h:%d dilation_w:%d mode:%d computeType:%d\n",
      res, convDesc, pad_h, pad_w, u, v, dilation_h, dilation_w, mode,
      computeType);
  return res;
}

EXAPI etdnnStatus_t
cudnnCreateActivationDescriptor(etdnnActivationDescriptor_t *activationDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnCreateActivationDescriptor", etdnnCreateActivationDescriptor,
      activationDesc);
  interceptor_log(
      "cudnnCreateActivationDescriptor status:%d activationDesc:%p\n", res,
      *activationDesc);
  return res;
}

EXAPI etdnnStatus_t
cudnnDestroyActivationDescriptor(etdnnActivationDescriptor_t activationDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDestroyActivationDescriptor", etdnnDestroyActivationDescriptor,
      activationDesc);
  interceptor_log(
      "cudnnDestroyActivationDescriptor status:%d activationDesc:%p\n", res,
      activationDesc);
  return res;
}

EXAPI etdnnStatus_t cudnnSetActivationDescriptor(
    etdnnActivationDescriptor_t activationDesc, etdnnActivationMode_t mode,
    etdnnNanPropagation_t reluNanOpt, double coef) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetActivationDescriptor", etdnnSetActivationDescriptor,
      activationDesc, mode, reluNanOpt, coef);
  interceptor_log("cudnnSetActivationDescriptor status:%d activationDesc:%p "
                  "mode:%d reluNanOpt:%d coef:%f\n",
                  res, activationDesc, mode, reluNanOpt, coef);
  return res;
}

EXAPI etdnnStatus_t cudnnActivationForward(
    etdnnHandle_t handle, etdnnActivationDescriptor_t activationDesc,
    const void *alpha, const etdnnTensorDescriptor_t xDesc, const void *x,
    const void *beta, const etdnnTensorDescriptor_t yDesc, void *y) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnActivationForward", etdnnActivationForward, handle, activationDesc,
      alpha, xDesc, x, beta, yDesc, y);
  interceptor_log(
      "cudnnActivationForward status:%d handle:%p activationDesc:%p alpha:%p "
      "xDesc:%p x:%p beta:%p yDesc:%p y:%p\n",
      res, handle, activationDesc, alpha, xDesc, x, beta, yDesc, y);
  return res;
}

EXAPI etdnnStatus_t
cudnnCreatePoolingDescriptor(etdnnPoolingDescriptor_t *poolingDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnCreatePoolingDescriptor", etdnnCreatePoolingDescriptor,
      poolingDesc);
  interceptor_log("cudnnCreatePoolingDescriptor status:%d poolingDesc:%p\n",
                  res, *poolingDesc);
  return res;
}

EXAPI etdnnStatus_t
cudnnDestroyPoolingDescriptor(etdnnPoolingDescriptor_t poolingDesc) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDestroyPoolingDescriptor", etdnnDestroyPoolingDescriptor,
      poolingDesc);
  interceptor_log("cudnnDestroyPoolingDescriptor status:%d poolingDesc:%p\n",
                  res, poolingDesc);
  return res;
}

EXAPI etdnnStatus_t cudnnSetPooling2dDescriptor(
    etdnnPoolingDescriptor_t poolingDesc, etdnnPoolingMode_t mode,
    etdnnNanPropagation_t maxpoolingNanOpt, int windowHeight, int windowWidth,
    int verticalPadding, int horizontalPadding, int verticalStride,
    int horizontalStride) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetPooling2dDescriptor", etdnnSetPooling2dDescriptor, poolingDesc,
      mode, maxpoolingNanOpt, windowHeight, windowWidth, verticalPadding,
      horizontalPadding, verticalStride, horizontalStride);
  interceptor_log(
      "cudnnSetPooling2dDescriptor status:%d poolingDesc:%p mode:%d "
      "maxpoolingNanOpt:%d windowHeight:%d windowWidth:%d verticalPadding:%d "
      "horizontalPadding:%d verticalStride:%d horizontalStride:%d\n",
      res, poolingDesc, mode, maxpoolingNanOpt, windowHeight, windowWidth,
      verticalPadding, horizontalPadding, verticalStride, horizontalStride);
  return res;
}

EXAPI etdnnStatus_t cudnnPoolingForward(
    etdnnHandle_t handle, const etdnnPoolingDescriptor_t poolingDesc,
    const void *alpha, const etdnnTensorDescriptor_t xDesc, const void *x,
    const void *beta, const etdnnTensorDescriptor_t yDesc, void *y) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnPoolingForward", etdnnPoolingForward, handle, poolingDesc, alpha,
      xDesc, x, beta, yDesc, y);
  interceptor_log("cudnnPoolingForward status:%d handle:%p poolingDesc:%p "
                  "alpha:%p xDesc:%p x:%p beta:%p yDesc:%p y:%p\n",
                  res, handle, poolingDesc, alpha, xDesc, x, beta, yDesc, y);
  return res;
}

EXAPI etdnnStatus_t cudnnAddTensor(etdnnHandle_t handle, const void *alpha,
                                   const etdnnTensorDescriptor_t aDesc,
                                   const void *A, const void *beta,
                                   const etdnnTensorDescriptor_t cDesc,
                                   void *C) {
  etdnnStatus_t res =
      InterceptFunction<etdnnStatus_t>("cudnnAddTensor", etdnnAddTensor, handle,
                                       alpha, aDesc, A, beta, cDesc, C);
  interceptor_log("cudnnAddTensor status:%d handle:%p alpha:%p aDesc:%p A:%p "
                  "beta:%p cDesc:%p C:%p\n",
                  res, handle, alpha, aDesc, A, beta, cDesc, C);
  return res;
}

/*
    This function initializes a previously created generic Tensor descriptor
   object.

    Note: The total size of a tensor including the potential padding between
   dimensions is limited to 2 Giga-elements of type datatype. Tensors are
   restricted to having at least 4 dimensions, and at most CUDNN_DIM_MAX
   dimensions (defined in cudnn.h). When working with lower dimensional data, it
   is recommended that the user create a 4D tensor, and set the size along
   unused dimensions to 1. Parameters

    tensorDesc
    Input/Output. Handle to a previously created tensor descriptor.

    datatype
    Input. Data type.

    nbDims
    Input. Dimension of the tensor. Note: Do not use 2 dimensions.

    dimA
    Input. Array of dimension nbDims that contain the size of the tensor for
   every dimension. Size along unused dimensions should be set to 1.

    strideA
    Input. Array of dimension nbDims that contain the stride of the tensor for
   every dimension.

    The possible error values returned by this function and their meanings are
   listed below.

    Returns

    CUDNN_STATUS_SUCCESS
    The object was set successfully.

    CUDNN_STATUS_BAD_PARAM
    At least one of the elements of the array dimA was negative or zero, or
   dataType has an invalid enumerant value.

    CUDNN_STATUS_NOT_SUPPORTED
    The parameter nbDims is outside the range [4, CUDNN_DIM_MAX], or the total
   size of the tensor descriptor exceeds the maximim limit of 2 Giga-elements.
 */
EXAPI etdnnStatus_t cudnnSetTensorNdDescriptor(
    etdnnTensorDescriptor_t tensorDesc, etdnnDataType_t dataType, int nbDims,
    const int *dimA, const int *strideA) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetTensorNdDescriptor", etdnnSetTensorNdDescriptor, tensorDesc,
      dataType, nbDims, dimA, strideA);
  interceptor_log("cudnnSetTensorNdDescriptor status:%d tensorDesc:%p "
                  "dataType:%d nbDims:%d\n",
                  res, tensorDesc, dataType, nbDims);
  return res;
}

/*
    This function initializes an n-D tensor descriptor.

    Parameters

    tensorDesc
    Output. Pointer to the tensor descriptor struct to be initialized.

    format
    Input. Tensor format.

    dataType
    Input. Tensor data type.

    nbDims
    Input. Tensor dimension size.

    dimA
    Input. Array containing size of each dimension.

    Returns

    CUDNN_STATUS_SUCCESS
    The function was successful.

    CUDNN_STATUS_BAD_PARAM
    Tensor descriptor was not allocated properly; or input parameters are not
   set correctly.

    CUDNN_STATUS_NOT_SUPPORTED
    Dimension size requested is larger than maximum dimension size supported.
 */
EXAPI etdnnStatus_t cudnnSetTensorNdDescriptorEx(
    etdnnTensorDescriptor_t tensorDesc, etdnnTensorFormat_t format,
    etdnnDataType_t dataType, int nbDims, const int *dimA) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetTensorNdDescriptorEx", etdnnSetTensorNdDescriptorEx, tensorDesc,
      format, dataType, nbDims, dimA);
  interceptor_log("cudnnSetTensorNdDescriptorEx status:%d tensorDesc:%p "
                  "format:%d dataType:%d nbDims:%d\n",
                  res, tensorDesc, format, dataType, nbDims);
  return res;
}

EXAPI etdnnStatus_t cudnnDeriveBNTensorDescriptor(
    etdnnTensorDescriptor_t derivedBnDesc, const etdnnTensorDescriptor_t xDesc,
    etdnnBatchNormMode_t mode) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnDeriveBNTensorDescriptor", etdnnDeriveBNTensorDescriptor,
      derivedBnDesc, xDesc, mode);
  interceptor_log("cudnnDeriveBNTensorDescriptor status:%d derivedBnDesc:%p "
                  "xDesc:%p mode:%d\n",
                  res, derivedBnDesc, xDesc, mode);
  return res;
}

EXAPI etdnnStatus_t cudnnBatchNormalizationForwardInference(
    etdnnHandle_t handle, etdnnBatchNormMode_t mode, const void *alpha,
    const void *beta, const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnTensorDescriptor_t yDesc, void *y,
    const etdnnTensorDescriptor_t bnScaleBiasMeanVarDesc, const void *bnScale,
    const void *bnBias, const void *estimatedMean,
    const void *estimatedVariance, double epsilon) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnBatchNormalizationForwardInference",
      etdnnBatchNormalizationForwardInference, handle, mode, alpha, beta, xDesc,
      x, yDesc, y, bnScaleBiasMeanVarDesc, bnScale, bnBias, estimatedMean,
      estimatedVariance, epsilon);
  interceptor_log("cudnnBatchNormalizationForwardInference status:%d handle:%p "
                  "mode:%d alpha:%p beta:%p xDesc:%p x:%p yDesc:%p y:%p"
                  " bnScaleBiasMeanVarDesc:%p bnScale:%p bnBias:%p "
                  "estimatedMean:%p estimatedVariance:%p epsilon:%f\n",
                  res, handle, mode, alpha, beta, xDesc, x, yDesc, y,
                  bnScaleBiasMeanVarDesc, bnScale, bnBias, estimatedMean,
                  estimatedVariance, epsilon);
  return res;
}

EXAPI etdnnStatus_t cudnnGetConvolutionForwardAlgorithm(
    etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc,
    const etdnnFilterDescriptor_t wDesc,
    const etdnnConvolutionDescriptor_t convDesc,
    const etdnnTensorDescriptor_t yDesc,
    etdnnConvolutionFwdPreference_t preference, size_t memoryLimitInBytes,
    etdnnConvolutionFwdAlgo_t *algo) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnGetConvolutionForwardAlgorithm",
      etdnnGetConvolutionForwardAlgorithm, handle, xDesc, wDesc, convDesc,
      yDesc, preference, memoryLimitInBytes, algo);
  interceptor_log("cudnnGetConvolutionForwardAlgorithm status:%d handle:%p "
                  "xDesc:%p wDesc:%p convDesc:%p yDesc:%p preference:%d "
                  "memoryLimitInBytes:%ld algo:%d\n",
                  res, handle, xDesc, wDesc, convDesc, yDesc, preference,
                  memoryLimitInBytes, *algo);
  return res;
}

EXAPI etdnnStatus_t cudnnGetConvolutionForwardWorkspaceSize(
    etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc,
    const etdnnFilterDescriptor_t wDesc,
    const etdnnConvolutionDescriptor_t convDesc,
    const etdnnTensorDescriptor_t yDesc, etdnnConvolutionFwdAlgo_t algo,
    size_t *sizeInBytes) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnGetConvolutionForwardWorkspaceSize",
      etdnnGetConvolutionForwardWorkspaceSize, handle, xDesc, wDesc, convDesc,
      yDesc, algo, sizeInBytes);
  interceptor_log(
      "cudnnGetConvolutionForwardWorkspaceSize status:%d handle:%p xDesc:%p "
      "wDesc:%p convDesc:%p yDesc:%p algo:%d sizeInBytes:%ld\n",
      res, handle, xDesc, wDesc, convDesc, yDesc, algo, *sizeInBytes);
  return res;
}

EXAPI etdnnStatus_t cudnnConvolutionForward(
    etdnnHandle_t handle, const void *alpha,
    const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnFilterDescriptor_t wDesc, const void *w,
    const etdnnConvolutionDescriptor_t convDesc, etdnnConvolutionFwdAlgo_t algo,
    void *workSpace, size_t workSpaceSizeInBytes, const void *beta,
    const etdnnTensorDescriptor_t yDesc, void *y) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnConvolutionForward", etdnnConvolutionForward, handle, alpha, xDesc,
      x, wDesc, w, convDesc, algo, workSpace, workSpaceSizeInBytes, beta, yDesc,
      y);
  interceptor_log(
      "cudnnConvolutionForward status:%d handle:%p alpha:%p xDesc:%p x:%p "
      "wDesc:%p w:%p convDesc:%p algo:%d workSpace:%p workSpaceSizeInBytes:%ld "
      "beta:%p yDesc:%p y:%p\n",
      res, handle, alpha, xDesc, x, wDesc, w, convDesc, algo, workSpace,
      workSpaceSizeInBytes, beta, yDesc, y);
  return res;
}

EXAPI etdnnStatus_t cudnnSoftmaxForward(
    etdnnHandle_t handle, etdnnSoftmaxAlgorithm_t algorithm,
    etdnnSoftmaxMode_t mode, const void *alpha,
    const etdnnTensorDescriptor_t xDesc, const void *x, const void *beta,
    const etdnnTensorDescriptor_t yDesc, void *y) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSoftmaxForward", etdnnSoftmaxForward, handle, algorithm, mode,
      alpha, xDesc, x, beta, yDesc, y);
  interceptor_log(
      "cudnnSoftmaxForward status = %d handle = %p algorithm = %d, mode = %d, "
      "alpha = %p, xDesc = %p, x = %p, beta = %p, yDesc = %p, y = %p\n",
      res, handle, algorithm, mode, alpha, xDesc, x, beta, yDesc, y);
  return res;
}

EXAPI etdnnStatus_t cudnnSetConvolutionGroupCount(
    etdnnConvolutionDescriptor_t convDesc, int groupCount) {
  etdnnStatus_t res = InterceptFunction<etdnnStatus_t>(
      "cudnnSetConvolutionGroupCount", etdnnSetConvolutionGroupCount, convDesc,
      groupCount);
  interceptor_log(
      "cudnnSetConvolutionGroupCount status = %d convDesc = %p gc = %d\n", res,
      convDesc, groupCount);
  return res;
}

INTERCEPTOR_STUB(cudnnGetConvolutionBackwardDataAlgorithm)
INTERCEPTOR_STUB(cudnnSetDropoutDescriptor)
INTERCEPTOR_STUB(cudnnSetLRNDescriptor)
INTERCEPTOR_STUB(cudnnGetConvolutionNdDescriptor)
INTERCEPTOR_STUB(cudnnGetRNNLinLayerMatrixParams)
INTERCEPTOR_STUB(cudnnFindConvolutionForwardAlgorithmEx)
INTERCEPTOR_STUB(cudnnSetFilterNdDescriptor)
INTERCEPTOR_STUB(cudnnFindConvolutionBackwardDataAlgorithmEx)
INTERCEPTOR_STUB(cudnnRNNForwardInference)
INTERCEPTOR_STUB(cudnnPoolingBackward)
INTERCEPTOR_STUB(cudnnDropoutGetReserveSpaceSize)
INTERCEPTOR_STUB(cudnnCreateDropoutDescriptor)
INTERCEPTOR_STUB(cudnnSetPoolingNdDescriptor)
INTERCEPTOR_STUB(cudnnActivationBackward)
INTERCEPTOR_STUB(cudnnRestoreDropoutDescriptor)
INTERCEPTOR_STUB(cudnnDropoutBackward)
INTERCEPTOR_STUB(cudnnGetRNNWorkspaceSize)
INTERCEPTOR_STUB(cudnnRNNBackwardWeights)
INTERCEPTOR_STUB(cudnnCreateRNNDescriptor)
INTERCEPTOR_STUB(cudnnGetConvolution2dDescriptor)
INTERCEPTOR_STUB(cudnnGetConvolutionBackwardFilterAlgorithm)
INTERCEPTOR_STUB(cudnnTransformTensor)
INTERCEPTOR_STUB(cudnnLRNCrossChannelForward)
INTERCEPTOR_STUB(cudnnRNNBackwardData)
INTERCEPTOR_STUB(cudnnFindConvolutionBackwardFilterAlgorithmEx)
INTERCEPTOR_STUB(cudnnRNNForwardTraining)
INTERCEPTOR_STUB(cudnnDropoutForward)
INTERCEPTOR_STUB(cudnnGetConvolutionBackwardDataWorkspaceSize)
INTERCEPTOR_STUB(cudnnSoftmaxBackward)
INTERCEPTOR_STUB(cudnnGetConvolutionBackwardFilterWorkspaceSize)
INTERCEPTOR_STUB(cudnnFindConvolutionForwardAlgorithm)
INTERCEPTOR_STUB(cudnnGetRNNLinLayerBiasParams)
INTERCEPTOR_STUB(cudnnLRNCrossChannelBackward)
INTERCEPTOR_STUB(cudnnSetConvolutionNdDescriptor)
INTERCEPTOR_STUB(cudnnDestroyDropoutDescriptor)
INTERCEPTOR_STUB(cudnnConvolutionBackwardData)
INTERCEPTOR_STUB(cudnnFindConvolutionBackwardDataAlgorithm)
INTERCEPTOR_STUB(cudnnGetRNNParamsSize)
INTERCEPTOR_STUB(cudnnCreateLRNDescriptor)
INTERCEPTOR_STUB(cudnnDestroyRNNDescriptor)
INTERCEPTOR_STUB(cudnnGetFilterNdDescriptor)
INTERCEPTOR_STUB(cudnnGetRNNTrainingReserveSize)
INTERCEPTOR_STUB(cudnnDropoutGetStatesSize)
INTERCEPTOR_STUB(cudnnConvolutionBackwardBias)
INTERCEPTOR_STUB(cudnnBatchNormalizationBackward)
INTERCEPTOR_STUB(cudnnFindConvolutionBackwardFilterAlgorithm)
INTERCEPTOR_STUB(cudnnDestroyLRNDescriptor)
INTERCEPTOR_STUB(cudnnConvolutionBackwardFilter)
INTERCEPTOR_STUB(cudnnSetRNNDescriptor)
INTERCEPTOR_STUB(cudnnBatchNormalizationForwardTraining)
INTERCEPTOR_STUB(cudnnSetConvolutionMathType)

////////////////////////////////////////////////////////////////////////////////////
// curand: /usr/local/cuda-9.1/targets/x86_64-linux/include/curand.h
////////////////////////////////////////////////////////////////////////////////////

INTERCEPTOR_STUB(curandCreateGenerator)
INTERCEPTOR_STUB(curandDestroyGenerator)
INTERCEPTOR_STUB(curandSetPseudoRandomGeneratorSeed)
INTERCEPTOR_STUB(curandSetStream)
INTERCEPTOR_STUB(curandGenerate)
INTERCEPTOR_STUB(curandGenerateUniform)
INTERCEPTOR_STUB(curandGenerateNormal)
INTERCEPTOR_STUB(curandGenerateNormalDouble)

////////////////////////////////////////////////////////////////////////////////////
// cu: /usr/local/cuda-9.1/targets/x86_64-linux/include/cuda.h
////////////////////////////////////////////////////////////////////////////////////

INTERCEPTOR_STUB(cuLaunchKernel)
INTERCEPTOR_STUB(cuModuleGetFunction)
INTERCEPTOR_STUB(cuModuleLoadDataEx)
INTERCEPTOR_STUB(cuModuleUnload)
INTERCEPTOR_STUB(cuGetErrorName)

////////////////////////////////////////////////////////////////////////////////////
// nvrtc: /usr/local/cuda-9.1/targets/x86_64-linux/include/nvrtc.h
////////////////////////////////////////////////////////////////////////////////////

INTERCEPTOR_STUB(nvrtcCreateProgram)
INTERCEPTOR_STUB(nvrtcDestroyProgram)
INTERCEPTOR_STUB(nvrtcCompileProgram)
INTERCEPTOR_STUB(nvrtcGetProgramLogSize)
INTERCEPTOR_STUB(nvrtcGetProgramLog)
INTERCEPTOR_STUB(nvrtcGetPTXSize)
INTERCEPTOR_STUB(nvrtcGetPTX)
INTERCEPTOR_STUB(nvrtcGetErrorString)
