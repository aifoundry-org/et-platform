#include "et_device.h"
#include "Core/Commands.h"
#include "Core/MemoryManager.h"
#include "demangle.h"
#include "registry.h"
#include "utils.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

// clang-format off
// et-rpc is an external dependency to be deprecated
// unfortunately the et-card-proxy.h header is not self
// contained and misisng includes
#include <stddef.h>
#include <stdint.h>
#include "et-rpc/et-card-proxy.h"
// clang-format on

using namespace et_runtime;
using namespace et_runtime::device;

void EtDevice::deviceThread() {
  // fprintf(stderr, "Hello from EtDevice::deviceThread()\n");

  CardProxy card_proxy_s;
  CardProxy *card_proxy = &card_proxy_s;
  const char *mode = getenv("ETT_MODE");
  mode = mode ? mode : "";
  if (!strcmp(mode, "0") || !strcmp(mode, "local")) {
    card_proxy = nullptr; // i.e. local mode
  } else if (!strcmp(mode, "dev") || !strcmp(mode, "device")) {
    cpOpen(card_proxy, true); // i.e. use real device
  } else if (!strcmp(mode, "card-emu")) {
    cpOpen(card_proxy, false); // i.e. connect to card-emu
  } else {
    cpOpen(card_proxy, false); // by default connect to card-emu for now
  }

  while (true) {
    std::unique_lock<std::mutex> lk(mutex_);

    while (true) {
      if (device_thread_exit_requested_) {
        if (card_proxy) {
          cpClose(card_proxy);
        }
        return;
      }

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
      lk.unlock();
      actionToExecute->execute(card_proxy);
      EtAction::decRefCounter(actionToExecute);
      lk.lock();
    }

    cond_var_.wait(lk);
  }
}

/**
 * Reset internal objects if user code has not destroyed them
 */
void EtDevice::uninitObjects() {
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

void EtDevice::initDeviceThread() {
  std::thread th(&EtDevice::deviceThread, this); // starting new thread
  device_thread_.swap(th); // move thread handler to class field
  assert(!device_thread_exit_requested_);
}

void EtDevice::uninitDeviceThread() {
  assert(!isLocked());
  {
    std::lock_guard<std::mutex> lk(mutex_);
    device_thread_exit_requested_ = true;
  }
  cond_var_.notify_one();
  device_thread_.join();
}

etrtError_t EtDevice::mallocHost(void **ptr, size_t size) {
  return mem_manager_->mallocHost(ptr, size);
}

etrtError_t EtDevice::freeHost(void *ptr) {
  return mem_manager_->freeHost(ptr);
}

etrtError_t EtDevice::malloc(void **devPtr, size_t size) {
  return mem_manager_->malloc(devPtr, size);
}

etrtError_t EtDevice::free(void *devPtr) { return mem_manager_->free(devPtr); }

etrtError_t
EtDevice::pointerGetAttributes(struct etrtPointerAttributes *attributes,
                               const void *ptr) {
  return mem_manager_->pointerGetAttributes(attributes, ptr);
}

etrtError_t EtDevice::setupArgument(const void *arg, size_t size,
                                    size_t offset) {

  std::vector<uint8_t> &buff = launch_confs_.back().args_buff;
  THROW_IF(offset && offset != align_up(buff.size(), size),
           "kernel code relies on argument natural alignment");
  size_t new_buff_size = std::max(buff.size(), offset + size);
  buff.resize(new_buff_size, 0);
  memcpy(&buff[offset], arg, size);

  return etrtSuccess;
}

etrtError_t EtDevice::launch(const void *func, const char *kernel_name) {
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

etrtError_t EtDevice::rawLaunch(et_runtime::Module *module,
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

etrtError_t EtDevice::moduleLoad(et_runtime::Module *module, const void *image,
                                 size_t image_size) {
  {

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
  }

  etrtStreamSynchronize(nullptr);

  {

    auto et_module = this->getModule(module);

    EtLoadedKernelsBin &loaded_kernels_bin = loaded_kernels_bin_[et_module];
    assert(loaded_kernels_bin.devPtr);
    assert(loaded_kernels_bin.actionEvent);
    assert(loaded_kernels_bin.actionEvent->isExecuted());
    // ELF is already loaded, free actionEvent
    EtAction::decRefCounter(loaded_kernels_bin.actionEvent);
    loaded_kernels_bin.actionEvent = nullptr;
  }

  return etrtSuccess;
}

etrtError_t EtDevice::moduleUnload(et_runtime::Module *module) {
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
