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
#include "ProfileEvent.h"
#include "ScopedProfileEvent.h"
#include "runtime/IRuntime.h"
#include <cstdio>
#include <device-layer/IDeviceLayer.h>
#include <elfio/elfio.hpp>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <memory>
#include <sstream>
using namespace rt;
using namespace rt::profiling;

RuntimeImp::RuntimeImp(dev::IDeviceLayer* deviceLayer)
  : deviceLayer_(deviceLayer) {
  for (int i = 0; i < deviceLayer_->getDevicesCount(); ++i) {
    devices_.emplace_back(DeviceId{i});
    queueHelpers_.emplace_back(QueueHelper{deviceLayer->getSubmissionQueuesCount(i)});
  }
  for (auto&& d : devices_) {
    memoryManagers_.insert(
      {d, MemoryManager{deviceLayer_->getDramBaseAddress(), deviceLayer_->getDramSize(), kMinAllocationSize}});
  }
  kernelParametersCache_ = std::make_unique<KernelParametersCache>(this);
  responseReceiver_ = std::make_unique<ResponseReceiver>(deviceLayer_, this);
}

std::vector<DeviceId> RuntimeImp::getDevices() {
  ScopedProfileEvent profileEvent(Class::GetDevices, profiler_);
  return devices_;
}

KernelId RuntimeImp::loadCode(DeviceId device, const void* data, size_t size) {
  ScopedProfileEvent profileEvent(Class::LoadCode, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  // allocate a buffer in the device to load the code
  auto deviceBuffer = mallocDevice(device, size);

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
      RT_DLOG(INFO) << "Found segment: " << segment->get_index() << " Offset: 0x" << std::hex << offset
                    << " Physical Address: 0x" << std::hex << loadAddress << " Mem Size: 0x" << memSize
                    << " Copying to address: 0x" << addr << " Entry: 0x" << entry << "\n";
      memcpyHostToDevice(sstream, reinterpret_cast<const uint8_t*>(data) + offset, reinterpret_cast<void*>(addr),
                         memSize);
    }
  }
  waitForStream(sstream);
  destroyStream(sstream);

  auto kernel = std::make_unique<Kernel>(device, deviceBuffer, entry - basePhysicalAddress);

  // store the ref
  auto kernelId = static_cast<KernelId>(nextKernelId_++);
  auto it = kernels_.find(kernelId);
  if (it != end(kernels_)) {
    throw Exception("Can't create kernel");
  }
  kernels_.emplace(kernelId, std::move(kernel));
  return kernelId;
}

void RuntimeImp::unloadCode(KernelId kernel) {
  ScopedProfileEvent profileEvent(Class::UnloadCode, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(kernels_, kernel);

  // free the buffer
  freeDevice(it->second->deviceId_, it->second->deviceBuffer_);

  // and remove the kernel
  kernels_.erase(it);
}

void* RuntimeImp::mallocDevice(DeviceId device, size_t size, int alignment) {
  ScopedProfileEvent profileEvent(Class::MallocDevice, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(memoryManagers_, device);
  // enforce size is multiple of alignment
  size = alignment * ((size + alignment - 1) / alignment);
  return static_cast<void*>(it->second.malloc(size, alignment));
}
void RuntimeImp::freeDevice(DeviceId device, void* buffer) {
  ScopedProfileEvent profileEvent(Class::FreeDevice, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(memoryManagers_, device);
  it->second.free(buffer);
}

StreamId RuntimeImp::createStream(DeviceId device) {
  ScopedProfileEvent profileEvent(Class::CreateStream, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto streamId = static_cast<StreamId>(nextStreamId_++);
  auto it = streams_.find(streamId);
  if (it != end(streams_)) {
    throw Exception("Can't create stream");
  }
  streams_.emplace(streamId, Stream{device, queueHelpers_[static_cast<int>(device)].nextQueue()});
  return streamId;
}

void RuntimeImp::destroyStream(StreamId stream) {
  ScopedProfileEvent profileEvent(Class::DestroyStream, profiler_);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);
  streams_.erase(it);
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const void* h_src, void* d_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_);
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);

  auto evt = eventManager_.getNextId();
  it->second.lastEventId_ = evt;
  auto device = it->second.deviceId_;
  auto vq = it->second.vq_;

  device_ops_api::device_ops_data_write_cmd_t cmd;
  cmd.dst_device_phy_addr = reinterpret_cast<uint64_t>(d_dst);
  // TODO this could/should change with this ticket SW-6256. At the moment I fill both fields even if we only use
  // virtual address, because I'm not sure what value the firmware uses
  cmd.src_host_phy_addr = cmd.src_host_virt_addr = reinterpret_cast<uint64_t>(h_src);
  cmd.size = size;

  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  cmd.command_info.flags = barrier ? 1 : 0;
  sendCommandMasterMinion(it->second, evt, cmd, lock);
  profileEvent.setEventId(evt);
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const void* d_src, void* h_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_);
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);

  auto evt = eventManager_.getNextId();
  it->second.lastEventId_ = evt;
  auto device = it->second.deviceId_;
  auto vq = it->second.vq_;

  device_ops_api::device_ops_data_read_cmd_t cmd;
  cmd.src_device_phy_addr = reinterpret_cast<uint64_t>(d_src);
  // TODO this could/should change with this ticket SW-6256. At the moment I fill both fields even if we only use
  // virtual address, because I'm not sure what value the firmware uses
  cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(h_dst);
  cmd.size = size;

  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  cmd.command_info.flags = barrier ? 1 : 0;
  sendCommandMasterMinion(it->second, evt, cmd, lock);
  profileEvent.setEventId(evt);
  return evt;
}

void RuntimeImp::waitForEvent(EventId event) {
  ScopedProfileEvent profileEvent(Class::WaitForEvent, profiler_, event);
  RT_DLOG(INFO) << "Waiting for event " << static_cast<int>(event) << " to be dispatched.";
  eventManager_.blockUntilDispatched(event);
  RT_DLOG(INFO) << "Finished wait for event " << static_cast<int>(event);
}

void RuntimeImp::waitForStream(StreamId stream) {
  ScopedProfileEvent profileEvent(Class::WaitForStream, profiler_, stream);
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream, "Invalid stream");
  auto evt = it->second.lastEventId_;
  lock.unlock();
  RT_DLOG(INFO) << "WaitForStream: Waiting for event " << static_cast<int>(evt);
  waitForEvent(evt);
}

std::vector<int> RuntimeImp::getDevicesWithEventsOnFly() const {
  auto events = eventManager_.getOnflyEvents();
  std::vector<int> busyDevices;
  for (auto& [key, s] : streams_) {
    (void)(key);
    if (std::find(begin(busyDevices), end(busyDevices), static_cast<int>(s.deviceId_)) == end(busyDevices) &&
        events.find(s.lastEventId_) != end(events)) {
      busyDevices.emplace_back(static_cast<int>(s.deviceId_));
    }
  }
  return busyDevices;
}

void RuntimeImp::onResponseReceived(const std::vector<std::byte>& response) {

  // check the response header
  auto header = reinterpret_cast<const device_ops_api::rsp_header_t*>(response.data());
  auto eventId = EventId{header->rsp_hdr.tag_id};

  ProfileEvent event(Type::Single, Class::KernelTimestamps);
  event.setEvent(eventId);
  auto fillEvent = [](ProfileEvent& evt, const auto& response) {
    evt.addExtra("cmd_wait_time", response.cmd_wait_time);
    evt.addExtra("cmd_execution_time", response.cmd_execution_time);
  };

  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_read_rsp_t*>(response.data());
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_write_rsp_t*>(response.data());
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    fillEvent(event, *r);
    kernelParametersCache_->releaseBuffer(eventId);
    if (r->status != device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED) {
    }
    break;
  }
  }
  profiler_.record(event);
  eventManager_.dispatch(eventId);
}
