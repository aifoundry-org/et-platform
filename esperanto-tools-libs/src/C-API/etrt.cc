#include "C-API/etrt.h"
#include "Core/Commands.h"
#include "EsperantoRuntime.h"
#include "Support/DeviceGuard.h"
#include "Support/HelperMacros.h"
#include "demangle.h"
#include "registry.h"

#include <assert.h>
#include <memory>
#include <stdio.h>
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
  auto device_res = deviceManager->getActiveDeviceID();
  if (!device_res) {
    return device_res.getError();
  }
  *device = *device_res;
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
    GetDev dev;

    return dev->memcpyAsync(dst, src, count, kind, stream);
}

EXAPI etrtError_t etrtMemcpy(void *dst, const void *src, size_t count,
                             enum etrtMemcpyKind kind) {
  etrtError_t res = etrtMemcpyAsync(dst, src, count, kind, 0);
  etrtStreamSynchronize(0);

  return res;
}

EXAPI etrtError_t etrtMemset(void *devPtr, int value, size_t count) {
  GetDev dev;
  return dev->memset(devPtr, value, count);
}

EXAPI etrtError_t etrtStreamSynchronize(etrtStream_t stream) {
  GetDev dev;
  return dev->streamSynchronize(stream);
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

EXAPI etrtError_t etrtModuleLoad(et_runtime::Module *module, const void *image,
                                 size_t image_size) {
  // FIXME enable
  // GetDev dev;
  // auto load_res = dev->moduleLoad(image, image_size);
  // if (!load_res) {
  //   return load_res.getError();
  // }
  // *module = *load_res.get();
  return etrtSuccess;
}

EXAPI etrtError_t etrtModuleUnload(et_runtime::Module *module) {
  GetDev dev;
  return dev->moduleUnload(module);
}

EXAPI etrtError_t etrtRawLaunch(et_runtime::Module *module,
                                const char *kernel_name, const void *args,
                                size_t args_size, etrtStream_t stream) {
  GetDev dev;
  return dev->rawLaunch(module, kernel_name, args, args_size, stream);
}
