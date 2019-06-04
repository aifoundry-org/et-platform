#include "Core/Device.h"
#include "Common/ErrorTypes.h"
#include "Core/Commands.h"
#include "Core/MemoryManager.h"
#include "Support/DeviceGuard.h"
#include "Support/Logging.h"
#include "demangle.h"
#include "registry.h"

#include <assert.h>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// FIXME remove the follwing
#include "C-API/etrt.h"

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

using namespace et_runtime;
using namespace et_runtime::device;

namespace et_runtime {
namespace device {

DECLARE_string(dev_target);

}
} // namespace et_runtime

Device::Device()
    : mem_manager_(std::unique_ptr<et_runtime::device::MemoryManager>(
          new et_runtime::device::MemoryManager(*this))) {
  auto target_type = DeviceTarget::deviceToCreate();
  target_device_ = DeviceTarget::deviceFactory(target_type, "test_path");
}

Device::~Device() {
  // Must stop device thread first in case it have non-empty streams
  uninitObjects();
}

etrtError Device::init() {
  initDeviceThread();
  mem_manager_->init();
  return etrtSuccess;
}

etrtError Device::resetDevice() {
  uninitDeviceThread();
  mem_manager_->deInit();
  return etrtSuccess;
}

bool Device::deviceAlive() { return target_device_->alive(); }

void Device::deviceExecute() {
    while (true) {
      EtAction *actionToExecute = nullptr;
      for (auto &it : stream_storage_) {
        EtStream *stream = it.get();
        if (!stream->noCommands()) {
          EtAction *action = stream->frontCommand();
          if (action->readyForExecution()) {
            actionToExecute = action;
            stream->popCommand();
            break;
          }
        }
      }

      // if there is no action then we are going to wait on condition variable
      if (actionToExecute == nullptr) {
        break;
      }

      // execute action without mutex
      actionToExecute->execute(this);
      EtAction::decRefCounter(actionToExecute);
    }

    if (!target_device_->alive()) {
      return;
    }

}

bool Device::setBootRom(const std::string &path) {
  bootrom_path_ = path;
  std::ifstream input(path.c_str(), std::ios::binary);
  // Read the input file in binary-form in the holder buffer.
  // Use the stread iterator to read out all the file data
  bootrom_data_ =
      decltype(bootrom_data_)(std::istreambuf_iterator<char>(input), {});
  return true;
}

/**
 * Reset internal objects if user code has not destroyed them
 */
void Device::uninitObjects() {
  for (auto &it : stream_storage_) {
    EtStream *stream = it.get();
    while (!stream->noCommands()) {
      EtAction *act = stream->frontCommand();
      stream->popCommand();
      EtAction::decRefCounter(act);
    }
  }
  for (auto &it : event_storage_) {
    EtEvent *event = it.get();
    event->resetAction();
  }
}

void Device::initDeviceThread() {

  RTINFO << "Starting Device Thread";
  if (!target_device_->init()) {
    RTERROR << "Failed to initialize device";
    std::terminate();
  }


}

void Device::uninitDeviceThread() {
  target_device_->deinit();
}

etrtError_t Device::mallocHost(void **ptr, size_t size) {
  return mem_manager_->mallocHost(ptr, size);
}

etrtError_t Device::freeHost(void *ptr) { return mem_manager_->freeHost(ptr); }

etrtError_t Device::malloc(void **devPtr, size_t size) {
  return mem_manager_->malloc(devPtr, size);
}

etrtError_t Device::free(void *devPtr) { return mem_manager_->free(devPtr); }

etrtError_t
Device::pointerGetAttributes(struct etrtPointerAttributes *attributes,
                             const void *ptr) {
  return mem_manager_->pointerGetAttributes(attributes, ptr);
}

etrtError_t Device::setupArgument(const void *arg, size_t size, size_t offset) {

  std::vector<uint8_t> &buff = launch_confs_.back().args_buff;
  THROW_IF(offset && offset != align_up(buff.size(), size),
           "kernel code relies on argument natural alignment");
  size_t new_buff_size = std::max(buff.size(), offset + size);
  buff.resize(new_buff_size, 0);
  memcpy(&buff[offset], arg, size);

  return etrtSuccess;
}

etrtError_t Device::launch(const void *func, const char *kernel_name) {
  GetDev dev;

  EtLaunchConf launch_conf = std::move(dev->launch_confs_.back());
  dev->launch_confs_.pop_back();

  uintptr_t kernel_entry_point = 0;
  if (func) {
    EtKernelInfo kernel_info = etrtGetKernelInfoByHostFun(func);
    assert(kernel_info.name == kernel_name);
    if (kernel_info.elf_p) {
      // We have kernel from registered Esperanto ELF binary, incorporated into
      // host binary. First, ensure ELF binary is loaded to device. Second, we
      // will launch kernel not by name, but by kernel entry point address.

      EtLoadedKernelsBin &loaded_kernels_bin =
          dev->loaded_kernels_bin_[kernel_info.elf_p];

      if (loaded_kernels_bin.devPtr == nullptr) {
        dev->malloc(&loaded_kernels_bin.devPtr, kernel_info.elf_size);

        dev->addAction(dev->defaultStream_,
                       new EtActionWrite(loaded_kernels_bin.devPtr,
                                         kernel_info.elf_p,
                                         kernel_info.elf_size));

        assert(loaded_kernels_bin.actionEvent == nullptr);
        loaded_kernels_bin.actionEvent = new EtActionEvent();
        loaded_kernels_bin.actionEvent->incRefCounter();

        dev->addAction(dev->defaultStream_, loaded_kernels_bin.actionEvent);
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

etrtError_t Device::rawLaunch(et_runtime::Module *module,
                              const char *kernel_name, const void *args,
                              size_t args_size, etrtStream_t stream) {
  GetDev dev;

  auto et_module = dev->getModule(module);

  EtLoadedKernelsBin &loaded_kernels_bin = dev->loaded_kernels_bin_[et_module];
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

ErrorOr<et_runtime::Module *> Device::moduleLoad(const void *image,
                                                 size_t image_size) {

  auto new_module = this->createModule();

  size_t parsed_elf_size;
  parse_elf(image, &parsed_elf_size, &new_module->kernel_offset,
            &new_module->raw_kernel_offset);
  assert(parsed_elf_size <= image_size);

  assert(!loaded_kernels_bin_.count(new_module));
  EtLoadedKernelsBin &loaded_kernels_bin = loaded_kernels_bin_[new_module];

  this->malloc(&loaded_kernels_bin.devPtr, image_size);

  this->addAction(defaultStream_, new EtActionWrite(loaded_kernels_bin.devPtr,
                                                    image, image_size));

  loaded_kernels_bin.actionEvent = new EtActionEvent();
  loaded_kernels_bin.actionEvent->incRefCounter();

  this->addAction(defaultStream_, loaded_kernels_bin.actionEvent);

  etrtStreamSynchronize(nullptr);

    assert(loaded_kernels_bin.devPtr);
    assert(loaded_kernels_bin.actionEvent);
    assert(loaded_kernels_bin.actionEvent->isExecuted());
    // ELF is already loaded, free actionEvent
    EtAction::decRefCounter(loaded_kernels_bin.actionEvent);
    loaded_kernels_bin.actionEvent = nullptr;

    return new_module;
}

etrtError_t Device::moduleUnload(et_runtime::Module *module) {
  auto et_module = this->getModule(module);

  // It is expected that user synchronize on all streams in which kernels from
  // this module was launched. So we just correct out data structures without
  // stream synchronization.
  EtLoadedKernelsBin &loaded_kernels_bin = loaded_kernels_bin_[et_module];
  assert(loaded_kernels_bin.devPtr);
  assert(loaded_kernels_bin.actionEvent == nullptr);

  this->free(loaded_kernels_bin.devPtr);
  loaded_kernels_bin_.erase(et_module);
  this->destroyModule(et_module);
  return etrtSuccess;
}
