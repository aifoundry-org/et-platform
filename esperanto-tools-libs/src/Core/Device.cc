#include "Core/Device.h"

#include "Common/ErrorTypes.h"
#include "Core/DeviceTarget.h"
#include "Core/ELFSupport.h"
#include "Core/Event.h"
#include "Core/MemoryManager.h"
#include "Core/Stream.h"
#include "DeviceAPI/Commands.h"
#include "DeviceFW/FWManager.h"
#include "ModuleManager.h"
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

Device::Device(int index)
    : device_index_(index), fw_manager_(std::make_unique<FWManager>()),
      mem_manager_(std::make_unique<et_runtime::device::MemoryManager>(*this)),
      module_manager_(std::make_unique<ModuleManager>()) {
  auto target_type = DeviceTarget::deviceToCreate();
  target_device_ = DeviceTarget::deviceFactory(target_type, index);
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
      std::shared_ptr<device_api::CommandBase> commandToExecute;
      for (auto &it : stream_storage_) {
        Stream *stream = it.get();
        if (!stream->noCommands()) {
          auto command = stream->frontCommand();
          commandToExecute = command;
          stream->popCommand();
          break;
        }
      }

      // if there is no action then we are going to wait on condition variable
      if (!commandToExecute) {
        break;
      }

      // execute action without mutex
      commandToExecute->execute(this);
    }

    if (!target_device_->alive()) {
      return;
    }

}

etrtError Device::loadFirmwareOnDevice() {
  return fw_manager_->firmware().loadOnDevice(target_device_.get());
}

bool Device::setFWFilePaths(const std::vector<std::string> &paths) {
  auto status = fw_manager_->firmware().setFWFilePaths(paths);
  if (!status) {
    return status;
  }
  status = fw_manager_->firmware().readFW();
  return status;
}

/**
 * Reset internal objects if user code has not destroyed them
 */
void Device::uninitObjects() {
  for (auto &it : stream_storage_) {
    Stream *stream = it.get();
    while (!stream->noCommands()) {
      auto act = stream->frontCommand();
      stream->popCommand();
    }
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

Stream *Device::defaultStream() const { return defaultStream_; }
Stream *Device::getStream(Stream *stream) {
  Stream *et_stream = reinterpret_cast<Stream *>(stream);
  if (et_stream == nullptr) {
    return defaultStream_;
  }
  assert(stl_count(stream_storage_, et_stream));
  return et_stream;
}

Stream *Device::defaultStream() { return defaultStream_; }

Event *Device::getEvent(Event *event) {
  Event *et_event = reinterpret_cast<Event *>(event);
  assert(stl_count(event_storage_, et_event));
  return et_event;
}

Stream *Device::createStream(bool is_blocking) {
  Stream *new_stream = new Stream(is_blocking);
  stream_storage_.emplace_back(new_stream);
  return new_stream;
}
void Device::destroyStream(Stream *et_stream) {
  assert(stl_count(stream_storage_, et_stream) == 1);
  stl_remove(stream_storage_, et_stream);
}
Event *Device::createEvent(bool disable_timing, bool blocking_sync) {
  Event *new_event = new Event(disable_timing, blocking_sync);
  event_storage_.emplace_back(new_event);
  return new_event;
}

void Device::destroyEvent(Event *et_event) {
  assert(stl_count(event_storage_, et_event) == 1);
  stl_remove(event_storage_, et_event);
}

void Device::addCommand(
    Stream *et_stream,
    std::shared_ptr<et_runtime::device_api::CommandBase> et_action) {
  // FIXME: all blocking streams can synchronize through EtActionEventWaiter
  if (et_stream->isBlocking()) {
    defaultStream_->addCommand(et_action);
  } else {
    et_stream->addCommand(et_action);
  }
  /// @todo the following should be removed once we move to a multi-threaded
  /// setup
  deviceExecute();
}

etrtError Device::mallocHost(void **ptr, size_t size) {
  return mem_manager_->mallocHost(ptr, size);
}

etrtError Device::streamSynchronize(Stream *stream) {
  Stream *et_stream = getStream(stream);

  auto event = std::make_shared<Event>();
  addCommand(et_stream,
             std::dynamic_pointer_cast<device_api::CommandBase>(event));
  auto future = event->getFuture();
  auto response = future.get();
  return response.error();
}

etrtError Device::memcpyAsync(void *dst, const void *src, size_t count,
                              enum etrtMemcpyKind kind, Stream *stream) {
  Stream *et_stream;

  et_stream = getStream(stream);

  if (kind == etrtMemcpyDefault) {
    // All addresses not in device address space count as host address even if
    // it was not created with MallocHost
    bool is_dst_host = isPtrAllocedHost(dst) || !isPtrInDevRegion(dst);
    bool is_src_host = isPtrAllocedHost(src) || !isPtrInDevRegion(src);
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

  switch (kind) {
  case etrtMemcpyHostToDevice: {
    addCommand(et_stream, std::shared_ptr<device_api::CommandBase>(
                              new device_api::WriteCommand(dst, src, count)));
  } break;
  case etrtMemcpyDeviceToHost: {
    addCommand(et_stream, std::shared_ptr<device_api::CommandBase>(
                              new device_api::ReadCommand(dst, src, count)));
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

etrtError Device::memcpy(void *dst, const void *src, size_t count,
                         enum etrtMemcpyKind kind) {
  etrtError res = etrtMemcpyAsync(dst, src, count, kind, 0);
  etrtStreamSynchronize(0);

  return res;
}

etrtError Device::memset(void *devPtr, int value, size_t count) {
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

etrtError Device::freeHost(void *ptr) { return mem_manager_->freeHost(ptr); }

etrtError Device::malloc(void **devPtr, size_t size) {
  return mem_manager_->malloc(devPtr, size);
}

etrtError Device::free(void *devPtr) { return mem_manager_->free(devPtr); }

etrtError Device::pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                       const void *ptr) {
  return mem_manager_->pointerGetAttributes(attributes, ptr);
}

et_runtime::Module &Device::createModule(const std::string &name) {
  auto res = module_manager_->createModule(name);
  return std::get<1>(res);
}

void Device::destroyModule(et_runtime::ModuleID module) {
  module_manager_->destroyModule(module);
}

etrtError Device::setupArgument(const void *arg, size_t size, size_t offset) {

  std::vector<uint8_t> &buff = launch_confs_.back().args_buff;
  THROW_IF(offset && offset != align_up(buff.size(), size),
           "kernel code relies on argument natural alignment");
  size_t new_buff_size = std::max(buff.size(), offset + size);
  buff.resize(new_buff_size, 0);
  ::memcpy(&buff[offset], arg, size);

  return etrtSuccess;
}

etrtError Device::launch(const void *func, const char *kernel_name) {
  abort();
  // FIXME disabled for now until this feature is used unit-tested and
  // exercised.
  /*
    EtLaunchConf launch_conf = std::move(dev->launch_confs_.back());
    launch_confs_.pop_back();

    uintptr_t kernel_entry_point = 0;
    if (func) {
      EtKernelInfo kernel_info = etrtGetKernelInfoByHostFun(func);
      assert(kernel_info.name == kernel_name);
      if (kernel_info.elf_p) {
        // We have kernel from registered Esperanto ELF binary, incorporated
    into
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
            {"void caffe2::MulBroadcast2Kernel<float, float>(float const*, float
    " "const*, float*, int, int, int)", "MulBroadcast2Kernel_Float_Float"},
            {"void caffe2::(anonymous "
             "namespace)::binary_add_kernel_broadcast<false, float, float>(float
    " "const*, float const*, float*, int, int, int)",
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
  */
  return etrtSuccess;
}

etrtError Device::rawLaunch(et_runtime::ModuleID module_id,
                            const char *kernel_name, const void *args,
                            size_t args_size, Stream *stream) {

  auto et_module_res = module_manager_->getModule(module_id);
  if (!et_module_res) {
    return et_module_res.getError();
  }

  auto module = et_module_res.get();

  if (!module->onDevice()) {
    return etrtErrorModuleNotOnDevice;
  }

  THROW_IF(!module->rawKernelExists(kernel_name),
           "No raw kernel found in module by kernel name.");

  auto entry_point_res = module->onDeviceKernelEntryPoint(kernel_name);
  assert(entry_point_res);

  uintptr_t kernel_entry_point = entry_point_res.get();

  std::vector<uint8_t> args_buff(args_size);
  ::memcpy(&args_buff[0], args, args_size);

  addCommand(getStream(stream), std::shared_ptr<device_api::CommandBase>(
                                    new device_api::LaunchCommand(
                                        dim3(0, 0, 0), dim3(0, 0, 0), args_buff,
                                        kernel_entry_point, kernel_name)));

  return etrtSuccess;
}

et_runtime::Module *Device::getModule(et_runtime::ModuleID mid) {
  auto res = module_manager_->getModule(mid);
  if (!res) {
    return nullptr;
  }
  return res.get();
}

ErrorOr<et_runtime::ModuleID> Device::moduleLoad(const std::string &name,
                                                 const std::string &path) {

  auto res = module_manager_->createModule(name);
  return module_manager_->loadOnDevice(std::get<0>(res), path, this);
}

etrtError Device::moduleUnload(et_runtime::ModuleID mid) {
  auto et_module_res = module_manager_->getModule(mid);
  if (!et_module_res) {
    return et_module_res.getError();
  }
  this->destroyModule(mid);
  return etrtSuccess;
}
