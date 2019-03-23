#include "etrt.h"
#include "demangle.h"
#include "et_device.h"
#include "registry.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>

EXAPI const char *etrtGetErrorString(etrtError_t error) {
  // CUDA returns "unrecognized error code" if the error code is not recognized.
  return error == etrtSuccess ? "no error" : "etrt unrecognized error code";
}

EXAPI etrtError_t etrtGetDeviceCount(int *count) {
  *count = 1;
  return etrtSuccess;
}

EXAPI etrtError_t etrtGetDeviceProperties(struct etrtDeviceProp *prop,
                                          int device) {
  assert(device == 0);
  *prop = etrtDevicePropDontCare;
  // some values from CUDA Runtime 9.1 on GTX1060 (sm_61)
  prop->major = 6;
  prop->minor = 1;
  strcpy(prop->name, "Esperanto emulation of GeForce GTX 1060 6GB");
  prop->totalGlobalMem = 6371475456;
  prop->maxThreadsPerBlock = 1024;
  return etrtSuccess;
}

EXAPI etrtError_t etrtGetDevice(int *device) {
  *device = 0;
  return etrtSuccess;
}

EXAPI etrtError_t etrtSetDevice(int device) {
  assert(device == 0);
  return etrtSuccess;
}

EXAPI etrtError_t etrtMallocHost(void **ptr, size_t size) {
  GetDev dev;
  *ptr = dev->host_mem_region->alloc(size);
  return etrtSuccess;
}

EXAPI etrtError_t etrtFreeHost(void *ptr) {
  GetDev dev;
  dev->host_mem_region->free(ptr);
  return etrtSuccess;
}

EXAPI etrtError_t etrtMalloc(void **devPtr, size_t size) {
  GetDev dev;
  *devPtr = dev->dev_mem_region->alloc(size);
  return etrtSuccess;
}

EXAPI etrtError_t etrtFree(void *devPtr) {
  GetDev dev;
  dev->dev_mem_region->free(devPtr);
  return etrtSuccess;
}

EXAPI etrtError_t etrtPointerGetAttributes(
    struct etrtPointerAttributes *attributes, const void *ptr) {
  GetDev dev;
  void *p = (void *)ptr;
  attributes->device = 0;
  attributes->isManaged = false;
  if (dev->isPtrAllocedHost(p)) {
    attributes->memoryType = etrtMemoryTypeHost;
    attributes->devicePointer = nullptr;
    attributes->hostPointer = p;
  } else if (dev->isPtrAllocedDev(p)) {
    attributes->memoryType = etrtMemoryTypeDevice;
    attributes->devicePointer = p;
    attributes->hostPointer = nullptr;
  } else {
    THROW("Unexpected pointer");
  }
  return etrtSuccess;
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

  dev->launch_confs.push_back(launch_conf);
  return etrtSuccess;
}

EXAPI etrtError_t etrtSetupArgument(const void *arg, size_t size,
                                    size_t offset) {
  GetDev dev;

  std::vector<uint8_t> &buff = dev->launch_confs.back().args_buff;
  THROW_IF(offset && offset != align_up(buff.size(), size),
           "kernel code relies on argument natural alignment");
  size_t new_buff_size = std::max(buff.size(), offset + size);
  buff.resize(new_buff_size, 0);
  memcpy(&buff[offset], arg, size);

  return etrtSuccess;
}

EXAPI etrtError_t etrtLaunch(const void *func, const char *kernel_name) {
  GetDev dev;

  EtLaunchConf launch_conf = std::move(dev->launch_confs.back());
  dev->launch_confs.pop_back();

  uintptr_t kernel_entry_point = 0;
  if (func) {
    EtKernelInfo kernel_info = etrtGetKernelInfoByHostFun(func);
    assert(kernel_info.name == kernel_name);
    if (kernel_info.elf_p) {
      // We have kernel from registered Esperanto ELF binary, incorporated into
      // host binary. First, ensure ELF binary is loaded to device. Second, we
      // will launch kernel not by name, but by kernel entry point address.

      EtLoadedKernelsBin &loaded_kernels_bin =
          dev->loaded_kernels_bin[kernel_info.elf_p];

      if (loaded_kernels_bin.devPtr == nullptr) {
        loaded_kernels_bin.devPtr =
            dev->kernels_dev_mem_region->alloc(kernel_info.elf_size);

        dev->addAction(dev->defaultStream,
                       new EtActionWrite(loaded_kernels_bin.devPtr,
                                         kernel_info.elf_p,
                                         kernel_info.elf_size));

        assert(loaded_kernels_bin.actionEvent == nullptr);
        loaded_kernels_bin.actionEvent = new EtActionEvent();
        loaded_kernels_bin.actionEvent->incRefCounter();

        dev->addAction(dev->defaultStream, loaded_kernels_bin.actionEvent);
      }

      if (loaded_kernels_bin.actionEvent) {
        if (loaded_kernels_bin.actionEvent->isExecuted()) {
          // ELF is already loaded, free actionEvent
          EtAction::decRefCounter(loaded_kernels_bin.actionEvent);
          loaded_kernels_bin.actionEvent = nullptr;
        } else {
          // ELF loading is in process, insert EtActionEventWaiter before
          // EtActionLaunch
          dev->addAction(
              launch_conf.etStream,
              new EtActionEventWaiter(loaded_kernels_bin.actionEvent));
        }
      }

      kernel_entry_point =
          (uintptr_t)loaded_kernels_bin.devPtr + kernel_info.offset;
    } else {
      // For this kernel we have no registered Esperanto ELF binary,
      // incorporated into host binary. Try map kernel into builtin kernel and
      // call it by name.

      static const std::map<std::string, const char *> kKernelRemapTable = {
          {"void caffe2::math::(anonymous namespace)::SetKernel<float>(int, "
           "float, float*)",
           "SetKernel_Float"},
          {"void caffe2::SigmoidKernel<float>(int, float const*, float*)",
           "SigmoidKernel_Float"},
          {"void caffe2::MulBroadcast2Kernel<float, float>(float const*, float "
           "const*, float*, int, int, int)",
           "MulBroadcast2Kernel_Float_Float"},
          {"void caffe2::(anonymous "
           "namespace)::binary_add_kernel_broadcast<false, float, float>(float "
           "const*, float const*, float*, int, int, int)",
           "BinAddKernelBroadcast_False_Float_Float"},
          {"caffe2::math::_Kernel_float_Add(int, float const*, float const*, "
           "float*)",
           "AddKernel_Float"}};

      kernel_name = kKernelRemapTable.at(demangle(kernel_name));
    }
  }

  dev->addAction(launch_conf.etStream,
                 new EtActionLaunch(launch_conf.gridDim, launch_conf.blockDim,
                                    launch_conf.args_buff, kernel_entry_point,
                                    kernel_name));
  return etrtSuccess;
}

EXAPI etrtError_t etrtModuleLoad(etrtModule_t *module, const void *image,
                                 size_t image_size) {
  {
    GetDev dev;

    EtModule *new_module = dev->createModule();
    *module = reinterpret_cast<etrtModule_t>(new_module);

    size_t parsed_elf_size;
    parse_elf(image, &parsed_elf_size, &new_module->kernel_offset,
              &new_module->raw_kernel_offset);
    assert(parsed_elf_size <= image_size);

    assert(!dev->loaded_kernels_bin.count(new_module));
    EtLoadedKernelsBin &loaded_kernels_bin =
        dev->loaded_kernels_bin[new_module];

    loaded_kernels_bin.devPtr = dev->kernels_dev_mem_region->alloc(image_size);

    dev->addAction(
        dev->defaultStream,
        new EtActionWrite(loaded_kernels_bin.devPtr, image, image_size));

    loaded_kernels_bin.actionEvent = new EtActionEvent();
    loaded_kernels_bin.actionEvent->incRefCounter();

    dev->addAction(dev->defaultStream, loaded_kernels_bin.actionEvent);
  }

  etrtStreamSynchronize(nullptr);

  {
    GetDev dev;

    EtModule *et_module = dev->getModule(*module);

    EtLoadedKernelsBin &loaded_kernels_bin = dev->loaded_kernels_bin[et_module];
    assert(loaded_kernels_bin.devPtr);
    assert(loaded_kernels_bin.actionEvent);
    assert(loaded_kernels_bin.actionEvent->isExecuted());
    // ELF is already loaded, free actionEvent
    EtAction::decRefCounter(loaded_kernels_bin.actionEvent);
    loaded_kernels_bin.actionEvent = nullptr;
  }

  return etrtSuccess;
}

EXAPI etrtError_t etrtModuleUnload(etrtModule_t module) {
  GetDev dev;

  EtModule *et_module = dev->getModule(module);

  // It is expected that user synchronize on all streams in which kernels from
  // this module was launched. So we just correct out data structures without
  // stream synchronization.
  EtLoadedKernelsBin &loaded_kernels_bin = dev->loaded_kernels_bin[et_module];
  assert(loaded_kernels_bin.devPtr);
  assert(loaded_kernels_bin.actionEvent == nullptr);

  dev->kernels_dev_mem_region->free(loaded_kernels_bin.devPtr);
  dev->loaded_kernels_bin.erase(et_module);
  dev->destroyModule(et_module);
  return etrtSuccess;
}

EXAPI etrtError_t etrtRawLaunch(etrtModule_t module, const char *kernel_name,
                                const void *args, size_t args_size,
                                etrtStream_t stream) {
  GetDev dev;

  EtModule *et_module = dev->getModule(module);

  EtLoadedKernelsBin &loaded_kernels_bin = dev->loaded_kernels_bin[et_module];
  assert(loaded_kernels_bin.devPtr);
  assert(loaded_kernels_bin.actionEvent == nullptr);

  THROW_IF(et_module->raw_kernel_offset.count(kernel_name) == 0,
           "No raw kernel found in module by kernel name.");
  uintptr_t kernel_entry_point = (uintptr_t)loaded_kernels_bin.devPtr +
                                 et_module->raw_kernel_offset.at(kernel_name);

  std::vector<uint8_t> args_buff(args_size);
  memcpy(&args_buff[0], args, args_size);

  dev->addAction(dev->getStream(stream),
                 new EtActionLaunch(dim3(0, 0, 0), dim3(0, 0, 0), args_buff,
                                    kernel_entry_point, kernel_name));
  return etrtSuccess;
}
