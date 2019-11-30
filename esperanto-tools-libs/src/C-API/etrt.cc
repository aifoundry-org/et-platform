//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/C-API/etrt.h"
#include "DeviceAPI/Commands.h"
#include "demangle.h"
#include "esperanto/runtime/Core/Event.h"
#include "esperanto/runtime/EsperantoRuntime.h"
#include "esperanto/runtime/Support/DeviceGuard.h"
#include "esperanto/runtime/Support/HelperMacros.h"

#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace et_runtime;

const char *etrtGetErrorString(enum etrtError error) {
  return et_runtime::Error::errorString(error);
}

enum etrtError etrtGetDeviceCount(int *count) {
  auto deviceManager = getDeviceManager();
  auto ret = deviceManager->getDeviceCount();
  if (!ret) {
    return ret.getError();
  }
  *count = *ret;
  return etrtSuccess;
}

enum etrtError etrtGetDeviceProperties(struct etrtDeviceProp *prop,
                                       int device) {
  auto deviceManager = getDeviceManager();
  auto ret = deviceManager->getDeviceInformation(device);
  if (!ret) {
    return ret.getError();
  }
  *prop = ret.get();
  return etrtSuccess;
}

enum etrtError etrtGetDevice(int *device) {
  // FIXME SW-256
  auto deviceManager = getDeviceManager();
  auto device_res = deviceManager->getActiveDeviceID();
  if (!device_res) {
    return device_res.getError();
  }
  *device = *device_res;
  return etrtSuccess;
}

enum etrtError etrtSetDevice(int device) {
  // FIXME SW-256
  assert(device == 0);
  auto deviceManager = getDeviceManager();
  deviceManager->setActiveDevice(device);
  return etrtSuccess;
}

enum etrtError etrtMallocHost(void **ptr, size_t size) {
  GetDev dev;
  return dev->mem_manager().mallocHost(ptr, size);
}

enum etrtError etrtFreeHost(void *ptr) {
  GetDev dev;
  return dev->mem_manager().freeHost(ptr);
}

enum etrtError etrtMalloc(void **devPtr, size_t size) {
  GetDev dev;
  return dev->mem_manager().malloc(devPtr, size);
}

enum etrtError etrtFree(void *devPtr) {
  GetDev dev;
  return dev->mem_manager().free(devPtr);
}

enum etrtError
etrtPointerGetAttributes(struct etrtPointerAttributes *attributes,
                         const void *ptr) {
  GetDev dev;
  return dev->mem_manager().pointerGetAttributes(attributes, ptr);
}

// FIXME re-enable when the inteface is stable
// enum etrtError etrtStreamCreateWithFlags(Stream **pStream, unsigned int
// flags) {
//   assert((flags & ~(ETRT_EVENT_FLAGS_STREAM_DEFAULT |
//                     ETRT_EVENT_FLAGS_STREAM_NON_BLOCKING)) == 0);

//   GetDev dev;

//   Stream *new_stream =
//       dev->createStream((flags & ETRT_EVENT_FLAGS_STREAM_NON_BLOCKING) == 0);
//   *pStream = reinterpret_cast<Stream *>(new_stream);
//   return etrtSuccess;
// }

// enum etrtError etrtStreamCreate(Stream **pStream) {
//   return etrtStreamCreateWithFlags(pStream, ETRT_MEM_ALLOC_HOST);
// }

// enum etrtError etrtStreamDestroy(Stream *stream) {
//   GetDev dev;

//   Stream *et_stream = dev->getStream(stream);
//   dev->destroyStream(et_stream);
//   return etrtSuccess;
// }

enum etrtError etrtMemcpyAsync(void *dst, const void *src, size_t count,
                               enum etrtMemcpyKind kind, Stream *stream) {
  GetDev dev;

  return dev->memcpyAsync(dst, src, count, kind, stream);
}

enum etrtError etrtMemcpy(void *dst, const void *src, size_t count,
                          enum etrtMemcpyKind kind) {
  enum etrtError res = etrtMemcpyAsync(dst, src, count, kind, 0);
  etrtStreamSynchronize(0);

  return res;
}

enum etrtError etrtMemset(void *devPtr, int value, size_t count) {
  GetDev dev;
  return dev->memset(devPtr, value, count);
}

enum etrtError etrtStreamSynchronize(Stream *stream) {
  GetDev dev;
  return dev->streamSynchronize(stream);
}

enum etrtError etrtGetLastError(void) { return etrtSuccess; }

enum etrtError etrtEventCreateWithFlags(Event **event, unsigned int flags) {
  assert((flags & ~(ETRT_EVENT_FLAGS_STREAM_DEFAULT |
                    ETRT_EVENT_FLAGS_STREAM_BLOCKING_SYNC |
                    ETRT_EVENT_FLAGS_STREAM_NON_BLOCKING |
                    ETRT_EVENT_FLAGS_INTERPROCESS |
                    ETRT_EVENT_FLAGS_DISABLE_TIMING)) == 0);

  GetDev dev;

  Event *new_event =
      dev->createEvent((flags & ETRT_EVENT_FLAGS_DISABLE_TIMING) != 0,
                       (flags & ETRT_EVENT_FLAGS_STREAM_BLOCKING_SYNC) != 0);
  *event = reinterpret_cast<Event *>(new_event);
  return etrtSuccess;
}

enum etrtError etrtEventCreate(Event **event) {
  return etrtEventCreateWithFlags(event, ETRT_EVENT_FLAGS_STREAM_DEFAULT);
}

enum etrtError etrtEventQuery(Event *event) {
  GetDev dev;

  Event *et_event = dev->getEvent(event);
  auto event_future = et_event->getFuture();
  auto event_response = event_future.get();
  return event_response.error();
}

// @todo FIXME the following is not tested
enum etrtError etrtEventRecord(Event *event, Stream *stream) {
  // GetDev dev;

  // Stream *et_stream = dev->getStream(stream);
  // Event *et_event = dev->getEvent(event);
  // dev->addAction(et_stream, event);
  return etrtSuccess;
}

enum etrtError etrtStreamWaitEvent(Stream *stream, Event *event,
                                   unsigned int flags) {
  // Must be zero by current API.
  assert(flags == 0);

  GetDev dev;

  Event *et_event = dev->getEvent(event);
  auto event_future = et_event->getFuture();
  event_future.wait();
  return etrtSuccess;
}

enum etrtError etrtEventSynchronize(Event *event) {
  // TODO: current implementation uses blocking semantics regardless of
  // cudaEventBlockingSync flag
  GetDev dev;

  Event *et_event = dev->getEvent(event);
  auto event_future = et_event->getFuture();
  event_future.wait();
  return etrtSuccess;
}

enum etrtError etrtEventElapsedTime(float *ms, Event *start, Event *end) {
  // TODO:
  assert(false);
  return etrtSuccess;
}

enum etrtError etrtEventDestroy(Event *event) {
  GetDev dev;

  Event *et_event = dev->getEvent(event);
  dev->destroyEvent(et_event);
  return etrtSuccess;
}

// FIXME SW-1362
// enum etrtError etrtConfigureCall(dim3 gridDim, dim3 blockDim, size_t
// sharedMem,
//                               Stream *stream) {
//   assert(sharedMem < 100 * 1024); // 100КВ; actually we allocate
//                                   // BLOCK_SHARED_REGION_TOTAL_SIZE - 64B

//   GetDev dev;

//   Stream *et_stream = dev->getStream(stream);

//   EtLaunchConf launch_conf;
//   launch_conf.gridDim = gridDim;
//   launch_conf.blockDim = blockDim;
//   launch_conf.etStream = et_stream;

//   dev->appendLaunchConf(launch_conf);
//   return etrtSuccess;
// }

// FIXME SW-1362
// enum etrtError etrtSetupArgument(const void *arg, size_t size, size_t offset)
// {
//   GetDev dev;
//   return dev->setupArgument(arg, size, offset);
// }

enum etrtError etrtLaunch(const void *func, const char *kernel_name) {
  // FIXME
  //  GetDev dev;
  //  return dev->launch(func, kernel_name);
  abort();
  return etrtSuccess;
}

enum etrtError etrtModuleLoad(et_runtime::CodeModuleID mid, const void *image,
                              size_t image_size) {
  abort();
  // FIXME enable
  // GetDev dev;
  // auto load_res = dev->moduleLoad(image, image_size);
  // if (!load_res) {
  //   return load_res.getError();
  // }
  // *module = *load_res.get();
  return etrtSuccess;
}

enum etrtError etrtModuleUnload(et_runtime::CodeModuleID mid) {
  GetDev dev;
  return dev->moduleUnload(mid);
}

enum etrtError etrtRawLaunch(et_runtime::CodeModuleID mid,
                             const char *kernel_name, const void *args,
                             size_t args_size, Stream *stream) {
  GetDev dev;
  // FIXME
  abort();
  //  return dev->rawLaunch(mid, kernel_name, args, args_size, stream);
  return etrtSuccess;
}
