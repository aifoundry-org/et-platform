/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "RuntimeImp.h"
#include "ExecutionContextCache.h"
#include "MemoryManager.h"
#include "ScopedProfileEvent.h"
#include "StreamManager.h"
#include "dma/DmaBufferImp.h"

#include "dma/HostBufferManager.h"
#include "runtime/IDmaBuffer.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"

#include <cstdint>
#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>

#include <elfio/elfio.hpp>

#include <chrono>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

namespace {
// these constants are based on documentation:
// https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1289355429/Device+Ops+Interface+-+Command+Response+Events+Bindings#Trace-setup-and-execution-flow
constexpr auto kTracingFwMmSize = 4 * (1 << 20);
constexpr auto kTracingFwCmSize = 1 * (1 << 20);
constexpr auto kTracingFwBufferSize = kTracingFwCmSize + kTracingFwMmSize;
constexpr int kEnableTracingBit = 1 << 0;
constexpr int kTraceRtControlLogToUart = 1 << 1;
constexpr int kResetTraceBufferAddressBit = 1 << 2; // Reset Trace buffer point to device side Trace Buffer base address
constexpr auto kNumErrorContexts = 2080;
constexpr auto kExceptionBufferSize = sizeof(ErrorContext) * kNumErrorContexts;
constexpr auto kNumExecutionCacheBuffers = 5; // initial number of execution cache buffers
} // namespace

RuntimeImp::~RuntimeImp() {
  for (auto d : devices_) {
    setMemoryManagerDebugMode(d, false);
  }
  running_ = false;
}

RuntimeImp::RuntimeImp(dev::IDeviceLayer* deviceLayer)
  : deviceLayer_(deviceLayer) {
  auto devicesCount = deviceLayer_->getDevicesCount();
  CHECK(devicesCount > 0);
  for (int i = 0; i < devicesCount; ++i) {
    auto id = DeviceId{i};
    devices_.emplace_back(id);
    auto sqCount = deviceLayer->getSubmissionQueuesCount(i);
    streamManager_.addDevice(id, sqCount);
    for (int sq = 0; sq < sqCount; ++sq) {
      commandSenders_.try_emplace(getCommandSenderIdx(i, sq), *deviceLayer_, i, sq);
    }
  }
  hostBufferManager_ = std::make_unique<HostBufferManager>(this, devices_[0]);
  auto dramBaseAddress = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  RT_LOG(INFO) << std::hex << "Runtime initialization. Dram base addr: " << dramBaseAddress
               << " Dram size: " << dramSize;
  for (auto& d : devices_) {
    memoryManagers_.try_emplace(d, dramBaseAddress, dramSize, kBlockSize);
    deviceTracing_.try_emplace(d, DeviceFwTracing{allocateDmaBuffer(d, kTracingFwBufferSize, false), nullptr, nullptr});
  }
  responseReceiver_ = std::make_unique<ResponseReceiver>(deviceLayer_, this);

  // initialization sequence, need to send abort command to ensure the device is in a proper state
  for (int d = 0; d < devicesCount; ++d) {
    RT_LOG(INFO) << "Initializing device: " << d;
    checkDevice(d);
    RT_LOG(INFO) << "Device: " << d << " initialized.";
  }
  eventManager_.setThrowOnMissingEvent(true);
  running_ = true;
  executionContextCache_ = std::make_unique<ExecutionContextCache>(
    this, kNumExecutionCacheBuffers, align(kExceptionBufferSize + kBlockSize, kBlockSize));
  RT_LOG(INFO) << "Runtime initialized.";
}

std::vector<DeviceId> RuntimeImp::getDevices() {
  ScopedProfileEvent profileEvent(Class::GetDevices, profiler_);
  return devices_;
}

LoadCodeResult RuntimeImp::loadCode(StreamId stream, const std::byte* data, size_t size) {
  ScopedProfileEvent profileEvent(Class::LoadCode, profiler_, stream);
  std::lock_guard lock(mutex_);

  auto stInfo = streamManager_.getStreamInfo(stream);

  auto memStream = std::istringstream(std::string(reinterpret_cast<const char*>(data), size), std::ios::binary);

  ELFIO::elfio elf;
  if (!elf.load(memStream)) {
    throw Exception("Error parsing elf");
  }

  auto extraSize = 0U;
  for (auto& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      auto fileSize = segment->get_file_size();
      auto memSize = segment->get_memory_size();
      extraSize += memSize - fileSize;
    }
  }

  // we need to add all the diff between fileSize and memSize to the final size
  // allocate a buffer in the device to load the code

  auto deviceBuffer = mallocDevice(DeviceId{stInfo.device_}, size + extraSize, kCacheLineSize);

  // copy the execution code into the device
  // iterate over all the LOAD segments, writing them to device memory
  uint64_t basePhysicalAddress;
  bool basePhysicalAddressCalculated = false;
  auto entry = elf.get_entry();

  std::vector<EventId> events;
  std::vector<std::unique_ptr<IDmaBuffer>> buffers;
  for (auto&& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      auto offset = segment->get_offset();
      auto loadAddress = segment->get_physical_address();
      auto fileSize = segment->get_file_size();
      auto memSize = segment->get_memory_size();
      auto addr = reinterpret_cast<uint64_t>(deviceBuffer) + offset;
      CHECK(memSize >= fileSize);
      if (!basePhysicalAddressCalculated) {
        basePhysicalAddress = loadAddress - offset;
        basePhysicalAddressCalculated = true;
        profileEvent.setLoadAddress(reinterpret_cast<uint64_t>(deviceBuffer)-basePhysicalAddress);
      }
      // allocate a dmabuffer to do the copy
      buffers.emplace_back(allocateDmaBuffer(DeviceId{stInfo.device_}, memSize, true));
      auto currentBuffer = buffers.back().get();
      // first fill with fileSize
      std::copy(data + offset, data + offset + fileSize, currentBuffer->getPtr());
      if (memSize > fileSize) {
        RT_VLOG(LOW) << "Memsize of segment " << segment->get_index() << " is larger than fileSize. Filling with 0s";
        std::fill_n(reinterpret_cast<uint8_t*>(currentBuffer->getPtr()) + fileSize, memSize - fileSize, 0);
      }
      RT_VLOG(LOW) << "S: " << segment->get_index() << std::hex << " O: 0x" << offset << " PA: 0x" << loadAddress
                   << " MS: 0x" << memSize << " FS: 0x" << fileSize << " @: 0x" << addr << " E: 0x" << entry << "\n";
      events.emplace_back(
        memcpyHostToDevice(stream, currentBuffer, reinterpret_cast<std::byte*>(addr), memSize, false));
    }
  }
  if (!basePhysicalAddressCalculated) {
    throw Exception("Error calculating kernel entrypoint");
  }

  auto kernel = std::make_unique<Kernel>(DeviceId{stInfo.device_}, deviceBuffer, entry - basePhysicalAddress);

  // store the ref
  auto kernelId = static_cast<KernelId>(nextKernelId_++);
  profileEvent.setKernelId(kernelId);
  auto it = kernels_.find(kernelId);
  if (it != end(kernels_)) {
    throw Exception("Can't create kernel");
  }
  kernels_.emplace(kernelId, std::move(kernel));

  // fill the struct results
  LoadCodeResult loadCodeResult;
  loadCodeResult.loadAddress_ = deviceBuffer;
  loadCodeResult.kernel_ = kernelId;
  if (events.size() == 1) {
    loadCodeResult.event_ = events.back();
  } else {
    loadCodeResult.event_ = eventManager_.getNextId();
    blockableThreadPool_.pushTask([this, evt = loadCodeResult.event_, eventsToWait = std::move(events)] {
      for (auto e : eventsToWait) {
        waitForEvent(e);
      }
      eventManager_.dispatch(evt);
    });
  }
  // add another thread to dispatch the buffers once the copy is done
  blockableThreadPool_.pushTask(
    [this, buffers = std::move(buffers), evt = loadCodeResult.event_] { waitForEvent(evt); });
  return loadCodeResult;
}

void RuntimeImp::unloadCode(KernelId kernel) {
  ScopedProfileEvent profileEvent(Class::UnloadCode, profiler_, kernel);
  std::lock_guard lock(mutex_);
  auto it = find(kernels_, kernel);
  auto deviceId = it->second->deviceId_;
  auto deviceBuffer = it->second->deviceBuffer_;
  RT_VLOG(LOW) << "Unloading kernel from deviceId " << static_cast<std::underlying_type_t<DeviceId>>(deviceId)
               << " buffer: " << deviceBuffer;

  // free the buffer
  freeDevice(deviceId, it->second->deviceBuffer_);

  // and remove the kernel
  kernels_.erase(it);
}

std::byte* RuntimeImp::mallocDevice(DeviceId device, size_t size, uint32_t alignment) {
  ScopedProfileEvent profileEvent(Class::MallocDevice, profiler_, device);
  RT_LOG(INFO) << "Malloc requested device " << std::hex << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " size: " << size << " alignment: " << alignment;

  if (__builtin_popcount(alignment) != 1) {
    throw Exception("Alignment must be power of two");
  }

  std::lock_guard lock(mutex_);
  auto it = find(memoryManagers_, device);
  return it->second.malloc(size, alignment);
}

void RuntimeImp::freeDevice(DeviceId device, std::byte* buffer) {
  ScopedProfileEvent profileEvent(Class::FreeDevice, profiler_, device);
  RT_LOG(INFO) << "Free at device: " << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " buffer address: " << std::hex << buffer;
  std::lock_guard lock(mutex_);
  find(memoryManagers_, device)->second.free(buffer);
}

StreamId RuntimeImp::createStream(DeviceId device) {
  ScopedProfileEvent profileEvent(Class::CreateStream, profiler_, device);
  RT_LOG(INFO) << "Creating stream at device: " << static_cast<std::underlying_type_t<DeviceId>>(device);
  return streamManager_.createStream(device);
}

void RuntimeImp::destroyStream(StreamId stream) {
  ScopedProfileEvent profileEvent(Class::DestroyStream, profiler_, stream);
  RT_LOG(INFO) << "Destroying stream: " << static_cast<std::underlying_type_t<StreamId>>(stream);
  streamManager_.destroyStream(stream);
}

bool RuntimeImp::waitForEvent(EventId event, std::chrono::seconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForEvent, profiler_, event);
  if (!running_) {
    RT_LOG(WARNING) << "Trying to wait for an event but runtime is not running anymore, returning.";
    return true;
  }

  RT_VLOG(LOW) << "Waiting for event " << static_cast<int>(event) << " to be dispatched.";
  auto res = eventManager_.blockUntilDispatched(event, timeout);
  RT_VLOG(LOW) << "Finished wait for event " << static_cast<int>(event) << " timed out? " << (res ? "false" : "true");
  return res;
}

bool RuntimeImp::waitForStream(StreamId stream, std::chrono::seconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForStream, profiler_, stream);
  auto events = streamManager_.getLiveEvents(stream);
  for (auto e : events) {
    RT_VLOG(LOW) << "WaitForStream: Waiting for event " << static_cast<int>(e);
    if (!waitForEvent(e, timeout)) {
      return false;
    }
  }
  return true;
}

void RuntimeImp::setOnStreamErrorsCallback(StreamErrorCallback callback) {
  streamManager_.setErrorCallback(std::move(callback));
}

void RuntimeImp::processResponseError(DeviceErrorCode errorCode, EventId event) {
  blockableThreadPool_.pushTask([this, errorCode, event] {
    // here we have to check if there is an associated errorbuffer with the event; if so, copy the buffer from
    // devicebuffer into dmabuffer; then do the callback
    StreamError streamError(errorCode);
    if (auto buffer = executionContextCache_->getReservedBuffer(event); buffer != nullptr) {
      // TODO remove this when ticket https://esperantotech.atlassian.net/browse/SW-9617 is fixed
      if (errorCode != DeviceErrorCode::KernelLaunchHostAborted) {
        // do the copy
        auto st = createStream(buffer->device_);
        auto errorContexts = std::vector<ErrorContext>(kNumErrorContexts);
        auto e = memcpyDeviceToHost(st, buffer->getExceptionContextPtr(),
                                    reinterpret_cast<std::byte*>(errorContexts.data()), kExceptionBufferSize, false);
        waitForEvent(e);
        streamError.errorContext_.emplace(std::move(errorContexts));
      }
      executionContextCache_->releaseBuffer(event);
    }
    if (!streamManager_.executeCallback(event, streamError)) {
      // the callback was not set, so add the error to the error list
      streamManager_.addError(event, std::move(streamError));
    }
    dispatch(event);
  });
}

void RuntimeImp::onResponseReceived(const std::vector<std::byte>& response) {

  // check the response header
  auto header = reinterpret_cast<const device_ops_api::rsp_header_t*>(response.data());
  auto eventId = EventId{header->rsp_hdr.tag_id};

  ProfileEvent event(Type::Instant, Class::KernelTimestamps);
  event.setEvent(eventId);
  auto fillEvent = [](ProfileEvent& evt, const auto& rsp) {
    RT_VLOG(HIGH) << std::hex << " Start time: " << rsp.device_cmd_start_ts << " Wait time: " << rsp.device_cmd_wait_dur
                  << " Execution time: " << rsp.device_cmd_execute_dur;
    evt.setDeviceCmdStartTs(rsp.device_cmd_start_ts);
    evt.setDeviceCmdWaitDur(rsp.device_cmd_wait_dur);
    evt.setDeviceCmdExecDur(rsp.device_cmd_execute_dur);
  };

  RT_VLOG(MID) << "Response received eventId: " << static_cast<int>(eventId)
               << " Message Id: " << header->rsp_hdr.msg_id;
  bool responseWasOk = true;
  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on kernel abort: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP:
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_read_rsp_t*>(response.data());
    fillEvent(event, *r);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA read: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on abort command: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP:
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_write_rsp_t*>(response.data());
    fillEvent(event, *r);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA write: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    fillEvent(event, *r);
    if (r->status !=
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on kernel launch: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    } else {
      executionContextCache_->releaseBuffer(eventId);
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_config_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_TRACE_RT_CONFIG_RESPONSE::DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on firmware trace configure: " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_control_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on firmware trace control (start/stop): " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(convert(header->rsp_hdr.msg_id, r->status), eventId);
    }
    break;
  default:
    RT_LOG(WARNING) << "Unknown response msg id: " << header->rsp_hdr.msg_id;
    break;
  }
  profiler_.record(event);
  // if response wasn't ok, then processResponseError will clean events
  if (responseWasOk) {
    dispatch(eventId);
  }
}

std::unique_ptr<IDmaBuffer> RuntimeImp::allocateDmaBuffer(DeviceId device, size_t size, bool writeable) {
  return std::make_unique<DmaBufferImp>(static_cast<int>(device), size, writeable, deviceLayer_);
}

EventId RuntimeImp::setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                                       uint32_t filterMask, bool barrier) {

  std::unique_lock lock(mutex_);

  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);

  std::vector<std::byte> cmdBase(sizeof(device_ops_api::device_ops_trace_rt_config_cmd_t));
  auto& cmd = *reinterpret_cast<device_ops_api::device_ops_trace_rt_config_cmd_t*>(cmdBase.data());
  std::memset(&cmd, 0, sizeof(cmd));
  cmd.event_mask = eventMask;
  cmd.filter_mask = filterMask;
  cmd.shire_mask = shireMask;
  cmd.thread_mask = threadMask;
  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  if (barrier)
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmdBase, commandSender, false, true});
  Sync(evt);
  return evt;
}

EventId RuntimeImp::startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput, bool barrier) {
  std::unique_lock lock(mutex_);
  if (!mmOutput && !cmOutput) {
    throw Exception("At least one output stream must be provided in order to record traces");
  }
  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);

  auto& deviceTracing = find(deviceTracing_, DeviceId{streamInfo.device_})->second;

  std::vector<std::byte> cmdBase(sizeof(device_ops_api::device_ops_trace_rt_control_cmd_t));
  auto& cmd = *reinterpret_cast<device_ops_api::device_ops_trace_rt_control_cmd_t*>(cmdBase.data());
  std::memset(&cmd, 0, sizeof(cmd));
  if (mmOutput)
    cmd.rt_type |= 1; // defined in host_iface.h
  if (cmOutput)
    cmd.rt_type |= 2; // defined in host_iface.h

  cmd.control |= kEnableTracingBit | kResetTraceBufferAddressBit;

  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  if (barrier)
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmdBase, commandSender, false, true});
  deviceTracing.cmOutput_ = cmOutput;
  deviceTracing.mmOutput_ = mmOutput;
  Sync(evt);
  return evt;
}

EventId RuntimeImp::stopDeviceTracing(StreamId stream, bool barrier) {
  std::unique_lock lock(mutex_);

  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);
  auto& deviceTracing = find(deviceTracing_, DeviceId{streamInfo.device_})->second;

  std::vector<std::byte> cmd(sizeof(device_ops_api::device_ops_data_read_cmd_t));
  auto cmdPtr = reinterpret_cast<device_ops_api::device_ops_data_read_cmd_t*>(cmd.data());
  std::memset(cmdPtr, 0, cmd.size());
  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmdPtr->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmdPtr->command_info.cmd_hdr.size = static_cast<device_ops_api::msg_size_t>(cmd.size());
  if (barrier) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  if (deviceTracing.mmOutput_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_MMFW_TRACEBUF;
    cmdPtr->size += kTracingFwMmSize;
  }
  if (deviceTracing.cmOutput_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_CMFW_TRACEBUF;
    cmdPtr->size += kTracingFwCmSize;
  }
  RT_VLOG(LOW) << "Retrieving device traces. Size: " << std::hex << cmdPtr->size;
  cmdPtr->dst_host_virt_addr = cmdPtr->dst_host_phy_addr =
    reinterpret_cast<uint64_t>(deviceTracing.dmaBuffer_->getPtr());

  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmd, commandSender, true, true});

  // replace the event (because we need to do the copy before dispatching the final event to the user)
  auto memcpyEvt = evt;
  evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);

  blockableThreadPool_.pushTask([this, memcpyEvt, dmaPtr = deviceTracing.dmaBuffer_->getPtr(),
                                 mmOut = deviceTracing.mmOutput_, cmOut = deviceTracing.cmOutput_, evt]() mutable {
    // first wait till the copy ends
    waitForEvent(memcpyEvt);

    if (mmOut) {
      mmOut->write(reinterpret_cast<const char*>(dmaPtr), kTracingFwMmSize);
      dmaPtr += kTracingFwMmSize;
    }
    if (cmOut) {
      cmOut->write(reinterpret_cast<const char*>(dmaPtr), kTracingFwCmSize);
    }
    dispatch(evt);
  });

  Sync(evt);
  return evt;
}

void RuntimeImp::setMemoryManagerDebugMode(DeviceId device, bool enable) {
  RT_LOG(INFO) << "Setting memory manager debug mode: " << (enable ? "True" : "False");
  find(memoryManagers_, device)->second.setDebugMode(enable);
}

void RuntimeImp::setSentCommandCallback(DeviceId device, CommandSender::CommandSentCallback callback) {
  auto deviceInt = static_cast<int>(device);
  auto sqCount = deviceLayer_->getSubmissionQueuesCount(deviceInt);
  for (auto sq = 0; sq < sqCount; ++sq) {
    auto& cs = find(commandSenders_, getCommandSenderIdx(deviceInt, sq))->second;
    cs.setOnCommandSentCallback(callback);
  }
}

std::vector<int> RuntimeImp::getDevicesWithEventsOnFly() const {
  std::vector<int> result;
  std::for_each(begin(devices_), end(devices_), [this, &result](const auto& device) {
    if (streamManager_.hasEventsOnFly(device)) {
      result.emplace_back(static_cast<int>(device));
    }
  });
  return result;
}

std::vector<StreamError> RuntimeImp::retrieveStreamErrors(StreamId stream) {
  return streamManager_.retrieveErrors(stream);
}

EventId RuntimeImp::abortCommand(EventId commandId) {
  using namespace std::chrono_literals;
  auto stInfo = streamManager_.getStreamInfo(commandId);
  auto evt = eventManager_.getNextId();
  if (!stInfo) {
    RT_LOG(WARNING) << "Trying to abort a command which was already finished. Runtime won't do anything.";
    dispatch(evt);
  } else {
    device_ops_api::device_ops_abort_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.tag_id = static_cast<uint16_t>(commandId);
    cmd.command_info.cmd_hdr.size = sizeof(cmd);
    cmd.command_info.cmd_hdr.flags = device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
    cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD;
    cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
    // check what device contains that eventid
    while (!deviceLayer_->sendCommandMasterMinion(stInfo->device_, stInfo->vq_, reinterpret_cast<std::byte*>(&cmd),
                                                  sizeof(cmd), false, true)) {
      RT_LOG(WARNING) << "Trying to abort a command but special HPSQ is full. Retrying...";
      std::this_thread::sleep_for(1ms);
    }
    streamManager_.addEvent(stInfo->id_, evt);
  }
  return evt;
}

EventId RuntimeImp::abortStream(StreamId streamId) {
  auto events = streamManager_.getLiveEvents(streamId);
  if (events.empty()) {
    RT_LOG(WARNING) << "Trying to abort stream " << static_cast<int>(streamId) << " but it had no outstanding events.";
    auto evt = eventManager_.getNextId();
    eventManager_.dispatch(evt);
    return evt;
  } else {
    auto lastEvt = EventId{};
    /* TODO
    for (auto e : events) {
      lastEvt = abortCommand(e);
    }*/
    // until this ticket get resolved, we need to abort just one command:
    // https://esperantotech.atlassian.net/browse/SW-9617
    lastEvt = abortCommand(events.back());
    RT_VLOG(LOW) << "Abort command event: " << static_cast<int>(lastEvt);
    return lastEvt;
  }
}

void RuntimeImp::dispatch(EventId event) {
  if (!running_) {
    RT_LOG(WARNING) << "Trying to dispatch an event but runtime is not running. Ignoring the dispatch."
                    << static_cast<int>(event);
    return;
  }
  streamManager_.removeEvent(event);
  eventManager_.dispatch(event);
}

void RuntimeImp::checkDevice(int device) {
  auto state = deviceLayer_->getDeviceStateMasterMinion(device);
  if (running_ && state == dev::DeviceState::PendingCommands) {
    return;
  }
  if (state != dev::DeviceState::Ready) {
    running_ = true; // we need to start running to allow dispatch and waitForStream to work properly
    RT_LOG(WARNING) << "Device " << device << " is not ready. Current state: " << static_cast<int>(state)
                    << ". Runtime will issue abort command to all SQs of that device.";
    for (auto sq = 0, sqCount = deviceLayer_->getSubmissionQueuesCount(device); sq < sqCount; ++sq) {
      auto st = createStream(DeviceId{device});
      auto fakeEvt = eventManager_.getNextId();
      streamManager_.addEvent(st, fakeEvt);
      abortCommand(fakeEvt);
      dispatch(fakeEvt);
      waitForStream(st);
      destroyStream(st);
    }
  }
}
