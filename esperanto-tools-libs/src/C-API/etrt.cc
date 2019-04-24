#include "C-API/etrt.h"
#include "Core/Commands.h"
#include "EsperantoRuntime.h"
#include "demangle.h"
#include "et_device.h"
#include "registry.h"
#include "utils.h"

#include <assert.h>
#include <memory>
#include <stdlib.h>

using namespace std;
using namespace et_runtime;

EXAPI const char *etrtGetErrorString(etrtError_t error) {
  return et_runtime::Error::errorString(error);
}

EXAPI etrtError_t etrtGetDeviceCount(int *count) {
  auto deviceManager = getDeviceManager();
  auto ret = deviceManager->getDeviceCount();
  if (!ret) {
    return ret.getError();
  }
  *count = *ret;
  return etrtSuccess;
}

EXAPI etrtError_t etrtGetDeviceProperties(struct etrtDeviceProp *prop,
                                          int device) {
  auto deviceManager = getDeviceManager();
  auto ret = deviceManager->getDeviceInformation(device);
  if (!ret) {
    return ret.getError();
  }
  *prop = ret.get();
  return etrtSuccess;
}

EXAPI etrtError_t etrtGetDevice(int *device) {
  // FIXME SW-256
  auto deviceManager = getDeviceManager();
  *device = deviceManager->getActiveDevice();
  return etrtSuccess;
}

EXAPI etrtError_t etrtSetDevice(int device) {
  // FIXME SW-256
  assert(device == 0);
  auto deviceManager = getDeviceManager();
  deviceManager->setActiveDevice(device);
  return etrtSuccess;
}

EXAPI etrtError_t etrtMallocHost(void **ptr, size_t size) {
  GetDev dev;
  return dev->mallocHost(ptr, size);
}

EXAPI etrtError_t etrtFreeHost(void *ptr) {
  GetDev dev;
  return dev->freeHost(ptr);
}

EXAPI etrtError_t etrtMalloc(void **devPtr, size_t size) {
  GetDev dev;
  return dev->malloc(devPtr, size);
}

EXAPI etrtError_t etrtFree(void *devPtr) {
  GetDev dev;
  return dev->free(devPtr);
}

EXAPI etrtError_t etrtPointerGetAttributes(
    struct etrtPointerAttributes *attributes, const void *ptr) {
  GetDev dev;
  return dev->pointerGetAttributes(attributes, ptr);
}

EXAPI etrtError_t etrtStreamCreateWithFlags(etrtStream_t *pStream,
                                            unsigned int flags) {
  assert((flags & ~(etrtStreamDefault | etrtStreamNonBlocking)) == 0);

  GetDev dev;

  EtStream *new_stream =
      dev->createStream((flags & etrtStreamNonBlocking) == 0);
  *pStream = reinterpret_cast<etrtStream_t>(new_stream);
  return etrtSuccess;
}

EXAPI etrtError_t etrtStreamCreate(etrtStream_t *pStream) {
  return etrtStreamCreateWithFlags(pStream, etrtHostAllocDefault);
}

EXAPI etrtError_t etrtStreamDestroy(etrtStream_t stream) {
  GetDev dev;

  EtStream *et_stream = dev->getStream(stream);
  dev->destroyStream(et_stream);
  return etrtSuccess;
}

EXAPI etrtError_t etrtMemcpyAsync(void *dst, const void *src, size_t count,
                                  enum etrtMemcpyKind kind,
                                  etrtStream_t stream) {
  EtStream *et_stream;

  {
    GetDev dev;
    et_stream = dev->getStream(stream);

    if (kind == etrtMemcpyDefault) {
      // All addresses not in device address space count as host address even if
      // it was not created with MallocHost
      bool is_dst_host =
          dev->isPtrAllocedHost(dst) || !dev->isPtrInDevRegion(dst);
      bool is_src_host =
          dev->isPtrAllocedHost(src) || !dev->isPtrInDevRegion(src);
      if (is_src_host) {
        if (is_dst_host) {
          kind = etrtMemcpyHostToHost;
        } else {
          kind = etrtMemcpyHostToDevice;
        }
      } else {
        if (is_dst_host) {
          kind = etrtMemcpyDeviceToHost;
        } else {
          kind = etrtMemcpyDeviceToDevice;
        }
      }
    }
  }

  switch (kind) {
  case etrtMemcpyHostToDevice: {
    GetDev dev;
    dev->addAction(et_stream, new EtActionWrite(dst, src, count));
  } break;
  case etrtMemcpyDeviceToHost: {
    GetDev dev;
    dev->addAction(et_stream, new EtActionRead(dst, src, count));
  } break;
  case etrtMemcpyDeviceToDevice: {
    int dev_count = count;
    const char *kern = "CopyKernel_Int8";

    if ((dev_count % 4) == 0) {
      dev_count /= 4;
      kern = "CopyKernel_Int32";
    }

    dim3 gridDim(defaultGridDim1D(dev_count));
    dim3 blockDim(defaultBlockDim1D());
    etrtConfigureCall(gridDim, blockDim, 0, stream);

    etrtSetupArgument(&dev_count, 4, 0);
    etrtSetupArgument(&src, 8, 8);
    etrtSetupArgument(&dst, 8, 16);
    etrtLaunch(nullptr, kern);
  } break;
  default:
    THROW("Unsupported Memcpy kind");
  }

  return etrtSuccess;
}

EXAPI etrtError_t etrtMemcpy(void *dst, const void *src, size_t count,
                             enum etrtMemcpyKind kind) {
  etrtError_t res = etrtMemcpyAsync(dst, src, count, kind, 0);
  etrtStreamSynchronize(0);

  return res;
}

EXAPI etrtError_t etrtMemset(void *devPtr, int value, size_t count) {
  const char *kern = "SetKernel_Int8";

  if ((count % 4) == 0) {
    count /= 4;
    kern = "SetKernel_Int32";
    value = (value & 0xff);
    value = value | (value << 8);
    value = value | (value << 16);
  }

  dim3 gridDim(defaultGridDim1D(count));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0, 0);

  etrtSetupArgument(&count, 4, 0);
  etrtSetupArgument(&value, 4, 4);
  etrtSetupArgument(&devPtr, 8, 8);
  etrtLaunch(nullptr, kern);
  etrtStreamSynchronize(0);

  return etrtSuccess;
}

EXAPI etrtError_t etrtStreamSynchronize(etrtStream_t stream) {
  EtActionEvent *actionEvent = nullptr;

  {
    GetDev dev;

    EtStream *et_stream = dev->getStream(stream);

    actionEvent = new EtActionEvent();
    actionEvent->incRefCounter();
    dev->addAction(et_stream, actionEvent);
  }

  actionEvent->observerWait();
  EtAction::decRefCounter(actionEvent);
  return etrtSuccess;
}

EXAPI etrtError_t etrtGetLastError(void) { return etrtSuccess; }

EXAPI etrtError_t etrtEventCreateWithFlags(etrtEvent_t *event,
                                           unsigned int flags) {
  assert((flags & ~(etrtEventDefault | etrtEventBlockingSync |
                    etrtEventDisableTiming | etrtEventInterprocess)) == 0);
  assert((flags & ~(etrtEventDisableTiming | etrtEventBlockingSync)) == 0);

  GetDev dev;

  EtEvent *new_event = dev->createEvent((flags & etrtEventDisableTiming) != 0,
                                        (flags & etrtEventBlockingSync) != 0);
  *event = reinterpret_cast<etrtEvent_t>(new_event);
  return etrtSuccess;
}

EXAPI etrtError_t etrtEventCreate(etrtEvent_t *event) {
  return etrtEventCreateWithFlags(event, etrtEventDefault);
}

EXAPI etrtError_t etrtEventQuery(etrtEvent_t event) {
  GetDev dev;

  EtEvent *et_event = dev->getEvent(event);
  EtActionEvent *actionEvent = et_event->getAction();
  if (actionEvent == nullptr) {
    return etrtSuccess;
  }

  return actionEvent->isExecuted() ? etrtSuccess : etrtErrorNotReady;
}

EXAPI etrtError_t etrtEventRecord(etrtEvent_t event, etrtStream_t stream) {
  GetDev dev;

  EtStream *et_stream = dev->getStream(stream);
  EtEvent *et_event = dev->getEvent(event);
  EtActionEvent *actionEvent = new EtActionEvent();
  et_event->resetAction(actionEvent);
  dev->addAction(et_stream, actionEvent);
  return etrtSuccess;
}

EXAPI etrtError_t etrtStreamWaitEvent(etrtStream_t stream, etrtEvent_t event,
                                      unsigned int flags) {
  // Must be zero by current API.
  assert(flags == 0);

  GetDev dev;

  EtStream *et_stream = dev->getStream(stream);
  EtEvent *et_event = dev->getEvent(event);
  EtActionEvent *actionEvent = et_event->getAction();
  if (actionEvent != nullptr) {
    dev->addAction(et_stream, new EtActionEventWaiter(actionEvent));
  }
  return etrtSuccess;
}

EXAPI etrtError_t etrtEventSynchronize(etrtEvent_t event) {
  // TODO: current implementation uses blocking semantics regardless of
  // cudaEventBlockingSync flag
  GetDev dev;

  EtEvent *et_event = dev->getEvent(event);
  EtActionEvent *actionEvent = et_event->getAction();
  if (actionEvent != nullptr) {
    actionEvent->observerWait();
    et_event->resetAction();
  }
  return etrtSuccess;
}

EXAPI etrtError_t etrtEventElapsedTime(float *ms, etrtEvent_t start,
                                       etrtEvent_t end) {
  // TODO:
  assert(false);
  return etrtSuccess;
}

EXAPI etrtError_t etrtEventDestroy(etrtEvent_t event) {
  GetDev dev;

  EtEvent *et_event = dev->getEvent(event);
  et_event->resetAction();
  dev->destroyEvent(et_event);
  return etrtSuccess;
}

EXAPI etrtError_t etrtConfigureCall(dim3 gridDim, dim3 blockDim,
                                    size_t sharedMem, etrtStream_t stream) {
  assert(sharedMem < 100 * 1024); // 100КВ; actually we allocate
                                  // BLOCK_SHARED_REGION_TOTAL_SIZE - 64B

  GetDev dev;

  EtStream *et_stream = dev->getStream(stream);

  EtLaunchConf launch_conf;
  launch_conf.gridDim = gridDim;
  launch_conf.blockDim = blockDim;
  launch_conf.etStream = et_stream;

  dev->appendLaunchConf(launch_conf);
  return etrtSuccess;
}

EXAPI etrtError_t etrtSetupArgument(const void *arg, size_t size,
                                    size_t offset) {
  GetDev dev;
  return dev->setupArgument(arg, size, offset);
}

EXAPI etrtError_t etrtLaunch(const void *func, const char *kernel_name) {
  GetDev dev;
  return dev->launch(func, kernel_name);
}

EXAPI etrtError_t etrtModuleLoad(etrtModule_t *module, const void *image,
                                 size_t image_size) {

  GetDev dev;
  return dev->moduleLoad(module, image, image_size);
}

EXAPI etrtError_t etrtModuleUnload(etrtModule_t module) {
  GetDev dev;
  return dev->moduleUnload(module);
}

EXAPI etrtError_t etrtRawLaunch(etrtModule_t module, const char *kernel_name,
                                const void *args, size_t args_size,
                                etrtStream_t stream) {
  GetDev dev;
  return dev->rawLaunch(module, kernel_name, args, args_size, stream);
}
