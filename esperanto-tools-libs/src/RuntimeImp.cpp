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
#include "KernelParametersCache.h"
#include "MemoryManager.h"
#include "ScopedProfileEvent.h"

#include "runtime/DmaBuffer.h"
#include "runtime/IRuntime.h"

#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>

#include <elfio/elfio.hpp>

#include <chrono>
#include <memory>
#include <sstream>
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
constexpr int kResetTraceBufferAddressBit = 1 << 2; // Reset Trace buffer point to device side Trace Buffer base address
} // namespace

RuntimeImp::~RuntimeImp() {
  RT_LOG(INFO) << "Destroying runtime";
  responseReceiver_.reset();
}

RuntimeImp::RuntimeImp(dev::IDeviceLayer* deviceLayer)
  : deviceLayer_(deviceLayer) {
  for (int i = 0; i < deviceLayer_->getDevicesCount(); ++i) {
    auto id = DeviceId{i};
    devices_.emplace_back(id);
    streamManager_.addDevice(id, deviceLayer->getSubmissionQueuesCount(i));
  }
  auto dramBaseAddress = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  RT_LOG(INFO) << std::hex << "Runtime initialization. Dram base addr: " << dramBaseAddress
               << " Dram size: " << dramSize;
  for (auto&& d : devices_) {
    memoryManagers_.try_emplace(d, MemoryManager{dramBaseAddress, dramSize, kBlockSize});
    dmaBufferManagers_.try_emplace(d, std::make_unique<DmaBufferManager>(deviceLayer_, static_cast<int>(d)));
    deviceTracing_.try_emplace(d, DeviceFwTracing{allocateDmaBuffer(d, kTracingFwBufferSize, false), nullptr, nullptr});
  }
  kernelParametersCache_ = std::make_unique<KernelParametersCache>(this);
  responseReceiver_ = std::make_unique<ResponseReceiver>(deviceLayer_, this);
}

std::vector<DeviceId> RuntimeImp::getDevices() {
  ScopedProfileEvent profileEvent(Class::GetDevices, profiler_);
  return devices_;
}

KernelId RuntimeImp::loadCode(DeviceId device, const void* data, size_t size) {
  ScopedProfileEvent profileEvent(Class::LoadCode, profiler_, device);
  std::unique_lock lock(mutex_);

  // allocate a buffer in the device to load the code
  auto deviceBuffer = mallocDevice(device, size, kCacheLineSize);

  auto memStream = std::istringstream(std::string(reinterpret_cast<const char*>(data), size), std::ios::binary);
  ELFIO::elfio elf;
  if (!elf.load(memStream)) {
    throw Exception("Error parsing elf");
  }

  // copy the execution code into the device
  // iterate over all the LOAD segments, writing them to device memory
  uint64_t basePhysicalAddress;
  bool basePhysicalAddressCalculated = false;
  auto entry = elf.get_entry();

  auto sstream = createStream(device);
  for (auto&& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      auto offset = segment->get_offset();
      auto loadAddress = segment->get_physical_address();
      auto memSize = segment->get_memory_size();
      auto addr = reinterpret_cast<uint64_t>(deviceBuffer) + offset;
      if (!basePhysicalAddressCalculated) {
        basePhysicalAddress = loadAddress - offset;
        basePhysicalAddressCalculated = true;
      }
      RT_VLOG(LOW) << "Found segment: " << segment->get_index() << std::hex << " Offset: 0x" << offset
                   << " Physical Address: 0x" << loadAddress << " Mem Size: 0x" << memSize << " Copying to address: 0x"
                   << addr << " Entry: 0x" << entry << "\n";
      memcpyHostToDevice(sstream, reinterpret_cast<const uint8_t*>(data) + offset, reinterpret_cast<void*>(addr),
                         memSize, false);
    }
  }
  if (!basePhysicalAddressCalculated) {
    throw Exception("Error calculating kernel entrypoint");
  }

  auto kernel = std::make_unique<Kernel>(device, deviceBuffer, entry - basePhysicalAddress);

  // store the ref
  auto kernelId = static_cast<KernelId>(nextKernelId_++);
  auto it = kernels_.find(kernelId);
  if (it != end(kernels_)) {
    throw Exception("Can't create kernel");
  }
  kernels_.emplace(kernelId, std::move(kernel));
  lock.unlock();
  waitForStream(sstream);
  destroyStream(sstream);
  return kernelId;
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

void* RuntimeImp::mallocDevice(DeviceId device, size_t size, uint32_t alignment) {
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

void RuntimeImp::freeDevice(DeviceId device, void* buffer) {
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

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const void* h_src, void* d_dst, size_t size, bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_, stream);
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);

  // TODO this is using a wrong command; to be fixed when implementing the DMA list:
  // https://esperantotech.atlassian.net/browse/SW-7830
  device_ops_api::device_ops_data_write_cmd_t cmd;
  std::memset(&cmd, 0, sizeof(cmd));
  cmd.dst_device_phy_addr = reinterpret_cast<uint64_t>(d_dst);

  // first check if its a DmaBuffer
  auto dmaBufferManager = find(dmaBufferManagers_, DeviceId{streamInfo.device_})->second.get();
  RT_VLOG(LOW) << "MemcpyHostToDevice stream: " << static_cast<std::underlying_type_t<StreamId>>(stream) << std::hex
               << " Host address: " << h_src << " Device address: " << cmd.dst_device_phy_addr << " Size: " << size;
  if (dmaBufferManager->isDmaBuffer(reinterpret_cast<const std::byte*>(h_src))) {
    cmd.src_host_virt_addr = cmd.src_host_phy_addr = reinterpret_cast<uint64_t>(h_src);
  } else {
    // if not, allocate a buffer, and stage the memory first into it
    auto tmpBuffer = dmaBufferManager->allocate(size, true);
    cmd.src_host_virt_addr = cmd.src_host_phy_addr = reinterpret_cast<uint64_t>(tmpBuffer.getPtr());
    RT_VLOG(LOW) << std::hex << "Staging memory transfer into a DmaBuffer to enable DMA. Dma address (actual address): "
                 << tmpBuffer.getPtr();

    // TODO: It can be copied in background and return control to the user. Future work.
    auto srcPtr = reinterpret_cast<const std::byte*>(h_src);
    std::copy(srcPtr, srcPtr + size, tmpBuffer.getPtr());

    // TODO: There are more efficient ways of achieving this. Future work.
    auto t = std::thread([this, evt, tmpBuffer = std::move(tmpBuffer)]() mutable {
      // wait till the command is acked
      waitForEvent(evt);
      // when exiting, tmpBuffer will be deallocated
    });
    t.detach();
  }
  cmd.size = static_cast<uint32_t>(size);

  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  if (barrier)
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, true);
  profileEvent.setEventId(evt);
  Sync(evt);
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const void* d_src, void* h_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_, stream);
  RT_VLOG(LOW) << "MemcpyDeviceToHost stream: " << static_cast<std::underlying_type_t<StreamId>>(stream) << std::hex
               << " Host address: " << h_dst << " Device address: " << d_src << " Size: " << size;
  std::unique_lock lock(mutex_);

  auto evt = eventManager_.getNextId();
  auto streamInfo = streamManager_.getStreamInfo(stream);
  streamManager_.addEvent(stream, evt);

  // TODO this is using a wrong command; to be fixed when implementing the DMA list:
  // https://esperantotech.atlassian.net/browse/SW-7830
  device_ops_api::device_ops_data_read_cmd_t cmd;
  std::memset(&cmd, 0, sizeof(cmd));
  cmd.size = static_cast<uint32_t>(size);
  cmd.src_device_phy_addr = reinterpret_cast<uint64_t>(d_src);
  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  if (barrier)
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;

  // first check if its a DmaBuffer
  auto dmaBufferManager = find(dmaBufferManagers_, DeviceId{streamInfo.device_})->second.get();
  if (dmaBufferManager->isDmaBuffer(reinterpret_cast<std::byte*>(h_dst))) {
    cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(h_dst);
    sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, true);
  } else {
    // if not, allocate a buffer, and copy the results from it to h_dst
    auto tmpBuffer = dmaBufferManager->allocate(size, false);
    RT_VLOG(LOW) << std::hex << "Staging memory transfer into a DmaBuffer to enable DMA. Dma address (actual address): "
                 << tmpBuffer.getPtr();
    cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(tmpBuffer.getPtr());
    sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, true);

    // TODO: There are many ways to optimize this, future work.
    // replace the event (because we need to do the copy before dispatching the final event to the user)
    auto memcpyEvt = evt;
    evt = eventManager_.getNextId();
    streamManager_.addEvent(stream, evt);

    auto t = std::thread([this, memcpyEvt, size, evt, h_dst, tmpBuffer = std::move(tmpBuffer)]() mutable {
      // first wait till the copy ends
      waitForEvent(memcpyEvt);
      // copy results to user buffer
      std::copy(tmpBuffer.getPtr(), tmpBuffer.getPtr() + size, reinterpret_cast<std::byte*>(h_dst));
      streamManager_.removeEvent(evt);
      // dispatch the event
      eventManager_.dispatch(evt);
    });
    t.detach();
  }

  profileEvent.setEventId(evt);
  Sync(evt);
  return evt;
}

bool RuntimeImp::waitForEvent(EventId event, std::chrono::seconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForEvent, profiler_, event);
  RT_VLOG(HIGH) << "Waiting for event " << static_cast<int>(event) << " to be dispatched.";
  auto res = eventManager_.blockUntilDispatched(event, timeout);
  RT_VLOG(HIGH) << "Finished wait for event " << static_cast<int>(event) << " timed out? " << (res ? "false" : "true");
  return res;
}

bool RuntimeImp::waitForStream(StreamId stream, std::chrono::seconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForStream, profiler_, stream);
  while (true) {
    auto lastEvent = streamManager_.getLastEvent(stream);

    if (lastEvent) {
      RT_VLOG(LOW) << "WaitForStream: Waiting for event " << static_cast<int>(lastEvent.value());
      if (!waitForEvent(lastEvent.value(), timeout)) {
        return false;
      }
    } else {
      return true;
    }
  }
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

  RT_VLOG(MID) << "Response received eventId: " << std::hex << static_cast<int>(eventId)
               << " Message Id: " << header->rsp_hdr.msg_id;
  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_read_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      std::stringstream ss;
      ss << "Error on DMA read: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      throw Exception(ss.str());
    }
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_write_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      std::stringstream ss;
      ss << "Error on DMA write: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      throw Exception(ss.str());
    }
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    fillEvent(event, *r);
    kernelParametersCache_->releaseBuffer(eventId);
    if (r->status !=
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED) {
      std::stringstream ss;
      ss << "Error on kernel launch: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      throw Exception(ss.str());
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_config_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_TRACE_RT_CONFIG_RESPONSE::DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS) {
      std::stringstream ss;
      ss << "Error on firmware trace configure: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      throw Exception(ss.str());
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_control_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS) {
      std::stringstream ss;
      ss << "Error on firmware trace control (start/stop): " << r->status << ". Tag id: " << static_cast<int>(eventId);
      throw Exception(ss.str());
    }
    break;
  }
  }
  profiler_.record(event);
  streamManager_.removeEvent(eventId);
  eventManager_.dispatch(eventId);
}

DmaBuffer RuntimeImp::allocateDmaBuffer(DeviceId device, size_t size, bool writeable) {
  std::lock_guard lock(mutex_);
  auto& dmaBufferManager = find(dmaBufferManagers_, device)->second;
  return dmaBufferManager->allocate(size, writeable);
}

EventId RuntimeImp::setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                                       uint32_t filterMask, bool barrier) {

  std::unique_lock lock(mutex_);

  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);

  device_ops_api::device_ops_trace_rt_config_cmd_t cmd;
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
  sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, false);
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

  device_ops_api::device_ops_trace_rt_control_cmd_t cmd;
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
  sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, false);
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

  device_ops_api::device_ops_data_read_cmd_t cmd;
  std::memset(&cmd, 0, sizeof(cmd));
  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  if (barrier) {
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  if (deviceTracing.mmOutput_) {
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_MMFW_TRACEBUF;
    cmd.size += kTracingFwMmSize;
  }
  if (deviceTracing.cmOutput_) {
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_CMFW_TRACEBUF;
    cmd.size += kTracingFwCmSize;
  }
  cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(deviceTracing.dmaBuffer_.getPtr());

  sendCommandMasterMinion(streamInfo.vq_, streamInfo.device_, cmd, lock, true);

  // TODO: There are many ways to optimize this, future work.
  // replace the event (because we need to do the copy before dispatching the final event to the user)
  auto memcpyEvt = evt;
  evt = eventManager_.getNextId();
  streamManager_.addEvent(stream, evt);
  auto t = std::thread([this, memcpyEvt, dmaPtr = deviceTracing.dmaBuffer_.getPtr(), mmOut = deviceTracing.mmOutput_,
                        cmOut = deviceTracing.cmOutput_, evt]() mutable {
    // first wait till the copy ends
    waitForEvent(memcpyEvt);

    if (mmOut) {
      mmOut->write(reinterpret_cast<const char*>(dmaPtr), kTracingFwMmSize);
      dmaPtr += kTracingFwMmSize;
    }
    if (cmOut) {
      cmOut->write(reinterpret_cast<const char*>(dmaPtr), kTracingFwCmSize);
    }
    streamManager_.removeEvent(evt);
    eventManager_.dispatch(evt);
  });
  t.detach();

  Sync(evt);
  return evt;
}

void RuntimeImp::setMemoryManagerDebugMode(DeviceId device, bool enable) {
  RT_LOG(INFO) << "Setting memory manager debug mode: " << (enable ? "True" : "False");
  find(memoryManagers_, device)->second.setDebugMode(enable);
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