#include "esperanto/runtime/Core/Device.h"

#include "CodeManagement/CodeModule.h"
#include "CodeManagement/ELFSupport.h"
#include "CodeManagement/ModuleManager.h"
#include "DeviceAPI/Commands.h"
#include "DeviceFW/FWManager.h"
#include "demangle.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/Common/ErrorTypes.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "esperanto/runtime/Core/Event.h"
#include "esperanto/runtime/Core/MemoryManager.h"
#include "esperanto/runtime/Core/Stream.h"
#include "esperanto/runtime/Support/DeviceGuard.h"
#include "esperanto/runtime/Support/Logging.h"

#include <absl/flags/flag.h>
#include <assert.h>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>


#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

ABSL_FLAG(std::string, shires, "2", "Number of active worker shires");

using namespace et_runtime;
using namespace et_runtime::device;

Device::Device(int index)
    : device_index_(index), fw_manager_(std::make_unique<FWManager>()),
      mem_manager_(std::make_unique<et_runtime::device::MemoryManager>(*this)) {
  auto target_type = DeviceTarget::deviceToCreate();
  target_device_ = DeviceTarget::deviceFactory(target_type, index);
  auto create_res = streamCreate(false);
  if (create_res.getError() != etrtSuccess) {
    std::terminate();
  }
  defaultStream_ = &create_res.get();
  stream_storage_.push_back(defaultStream_);
}

Device::~Device() {
  // Must stop device thread first in case it have non-empty streams
  uninitObjects();
}

etrtError Device::init() {
  initDeviceThread();
  mem_manager_->init();
  // Load the FW on the device
  auto success = loadFirmwareOnDevice();
  assert(success == etrtSuccess);
  auto res = target_device_->postFWLoadInit();
  assert(res);
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
        Stream *stream = it;
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
  // FIXME SW-364 enable correct memory initialization
  if (target_device_->type() == DeviceTarget::TargetType::PCIe) {
    return etrtSuccess;
  }
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
    Stream *stream = it;
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

Stream &Device::defaultStream() const { return *defaultStream_; }

Stream *Device::getStream(Stream *stream) {
  Stream *et_stream = reinterpret_cast<Stream *>(stream);
  if (et_stream == nullptr) {
    return defaultStream_;
  }
  assert(stl_count(stream_storage_, et_stream));
  return et_stream;
}

Event *Device::getEvent(Event *event) {
  Event *et_event = reinterpret_cast<Event *>(event);
  assert(stl_count(event_storage_, et_event));
  return et_event;
}

ErrorOr<Stream &> Device::streamCreate(bool blocking) {
  auto stream = new Stream(*this, blocking);
  // The stream object is being tracked by the global stream registry return
  // just a reference
  stream_storage_.push_back(stream);
  return *stream;
}
etrtError Device::destroyStream(StreamID id) {
  auto remove_registry = Stream::destroyStream(id);
  if (remove_registry != etrtSuccess) {
    return remove_registry;
  }
  stream_storage_.erase(
      std::remove_if(stream_storage_.begin(), stream_storage_.end(),
                     // remove any stream with a matching ID
                     [&id](Stream *x) { return x->id() == id; }),
      stream_storage_.end());
  return etrtSuccess;
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
    bool is_dst_host = mem_manager_->isPtrAllocatedHost(dst) || !mem_manager_->isPtrInDevRegion(dst);
    bool is_src_host = mem_manager_->isPtrAllocatedHost(src) || !mem_manager_->isPtrInDevRegion(src);
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
                              new device_api::pcie_commands::WriteCommand(
                                  dst, src, count)));
  } break;
  case etrtMemcpyDeviceToHost: {
    addCommand(et_stream, std::shared_ptr<device_api::CommandBase>(
                              new device_api::pcie_commands::ReadCommand(
                                  dst, src, count)));
  } break;
  case etrtMemcpyDeviceToDevice: {
    abort();
    /* FIXME SW-1293

    int dev_count = count;
    const char *kern = "CopyKernel_Int8";

    if ((dev_count % 4) == 0) {
      dev_count /= 4;
      kern = "CopyKernel_Int32";
    }

    dim3 gridDim(defaultGridDim1D(dev_count));
    dim3 blockDim(defaultBlockDim1D());
    EtLaunchConf launch_conf;
    launch_conf.gridDim = gridDim;
    launch_conf.blockDim = blockDim;
    launch_conf.etStream = nullptr;
    appendLaunchConf(launch_conf);
    setupArgument(&dev_count, 4, 0);
    setupArgument(&src, 8, 8);
    setupArgument(&dst, 8, 16);
    launch(nullptr, kern);
    */
  } break;
  default:
    THROW("Unsupported Memcpy kind");
  }
  return etrtSuccess;
}

etrtError Device::memcpy(void *dst, const void *src, size_t count,
                         enum etrtMemcpyKind kind) {
  etrtError res = memcpyAsync(dst, src, count, kind, 0);
  streamSynchronize(0);

  return res;
}

etrtError Device::memset(void *devPtr, int value, size_t count) {
  abort();
  /* FIXME SW-1293
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
  EtLaunchConf launch_conf;
  launch_conf.gridDim = gridDim;
  launch_conf.blockDim = blockDim;
  launch_conf.etStream = nullptr;
  appendLaunchConf(launch_conf);
  setupArgument(&count, 4, 0);
  setupArgument(&value, 4, 4);
  setupArgument(&devPtr, 8, 8);
  launch(nullptr, kern);
  etrtStreamSynchronize(0);
  */
  return etrtSuccess;
}

etrtError Device::moduleUnload(et_runtime::CodeModuleID mid) {
  /// FIXME SW-1370 correctly unload the module form the device
  return etrtSuccess;
}
