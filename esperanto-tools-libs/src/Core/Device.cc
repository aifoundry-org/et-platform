//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/Device.h"

#include "CodeManagement/CodeModule.h"
#include "CodeManagement/ELFSupport.h"
#include "CodeManagement/ModuleManager.h"
#include "Core/DeviceFwTypes.h"
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
      mem_manager_(std::make_unique<et_runtime::device::MemoryManager>(*this)),
      command_executor_(), terminate_cmd_executor_(false), device_reader_() {
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
  mem_manager_->init();
  initDeviceThread();
  // Load the FW on the device
  auto success = loadFirmwareOnDevice();
  assert(success == etrtSuccess);
  auto res = target_device_->postFWLoadInit();
  assert(res);
  // Initialize thread that is responsible for listening to mailbox
  // Resposes/Events. do that after the FW is loaded and the device
  // is initialized
  device_reader_ = std::thread([this]() { this->deviceListener(); });
  // Detach the device-reader it is expected to run in parallel and
  // consume any replies back from the device.
  device_reader_.detach();
  return etrtSuccess;
}

etrtError Device::resetDevice() {
  uninitDeviceThread();
  mem_manager_->deInit();
  return etrtSuccess;
}

bool Device::deviceAlive() { return target_device_->alive(); }

bool Device::addCommand(
    std::shared_ptr<et_runtime::device_api::CommandBase> &cmd) {
  {
    std::lock_guard<decltype(cmd_queue_mutex_)> guard(cmd_queue_mutex_);
    cmd_queue_.push(cmd);
  }
  queue_cv_.notify_one();
  return true;
}

bool Device::registerMBReadCallBack(et_runtime::device_api::CommandBase *cmd,
                                    const MBReadCallBack &cb) {
#if ENABLE_DEVICE_FW
  auto emplace_res = cb_map_.emplace(cmd->id(), std::forward_as_tuple(cmd, cb));
  return emplace_res.second;
#else
  return false;
#endif
}

void Device::deviceListener() {
#if ENABLE_DEVICE_FW
  while (true) {
    // Allocate a vector of the maximum mailbox message size to read
    // data in
    std::vector<uint8_t> message(target_device_->mboxMsgMaxSize(), 0);
    auto res = target_device_->mb_read(message.data(), message.size());
    if (res < 0) {
      RTERROR << "Error reading mailbox" << std::strerror(errno);
      continue;
    }
    if (!res) {
      RTERROR << "Error reading from the device mailbox \n";
      continue;
    }
    auto response =
        reinterpret_cast<::device_api::response_header_t *>(message.data());
    RTDEBUG << "MessageID: " << response->message_id << "\n";
    auto msgid = response->message_id;
    if (::device_api::MBOX_DEVAPI_MESSAGE_ID_NONE < msgid &&
        msgid < ::device_api::MBOX_DEVAPI_MESSAGE_ID_LAST) {
      auto rsp_header =
        reinterpret_cast<::device_api::response_header_t*>(message.data());
      auto it = cb_map_.find(rsp_header->command_info.command_id);
      assert(it != cb_map_.end());
      auto &[cmd, cb] = it->second;
      assert(cmd->commandTypeID() == rsp_header->command_info.message_id);
      auto res = cb(message);
      assert(res);
    } else {
      assert(false);
    }
  }
#endif
}

void Device::deviceExecute() {
  while (!terminate_cmd_executor_) {
    std::unique_lock<decltype(cmd_queue_mutex_)> lk(cmd_queue_mutex_);
    queue_cv_.wait(lk, [this]() {
      return !(cmd_queue_.empty() && !terminate_cmd_executor_);
    });

    if (cmd_queue_.size() > 0) {
      auto command = cmd_queue_.front();
      cmd_queue_.pop();

      command->execute(this);
    }
  }
  RTINFO << "Command Executor Thread Terminated";
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
  command_executor_ = std::thread([this]() { this->deviceExecute(); });
}

void Device::uninitDeviceThread() {
  // Notify the command executor thread to "wake" it up from the being blocked
  // waiting for a command to be inserted. The target_device should not be alive
  // any more and the executor thread shoulOBd terminate and us we should be
  // able to make that thread join.
  terminate_cmd_executor_ = true;
  queue_cv_.notify_one();
  command_executor_.join();

  /// Release the target device / or simulator
  target_device_->deinit();
}

Stream &Device::defaultStream() const { return *defaultStream_; }

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

etrtError Device::memcpyAsync(void *dst, const void *src, size_t count,
                              enum etrtMemcpyKind kind, Stream *stream) {
  Stream *et_stream = defaultStream_;

  if (stream != nullptr) {
    et_stream = stream;
  }

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
    et_stream->addCommand(std::shared_ptr<device_api::CommandBase>(
        new device_api::pcie_commands::WriteCommand(dst, src, count)));
  } break;
  case etrtMemcpyDeviceToHost: {
    et_stream->addCommand(std::shared_ptr<device_api::CommandBase>(
        new device_api::pcie_commands::ReadCommand(dst, src, count)));
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
  if (res != etrtSuccess) {
    return res;
  }
  res = defaultStream_->synchronize();
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
