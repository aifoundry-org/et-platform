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
#include "ScopedProfileEvent.h"

#include "runtime/DmaBuffer.h"
#include "runtime/IRuntime.h"

#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>

#include <elfio/elfio.hpp>

#include <chrono>
#include <cstdio>
#include <memory>
#include <sstream>

using namespace rt;
using namespace rt::profiling;

RuntimeImp::~RuntimeImp() {
  RT_LOG(INFO) << "Destroying runtime";
  responseReceiver_.reset();
}

RuntimeImp::RuntimeImp(dev::IDeviceLayer* deviceLayer)
  : deviceLayer_(deviceLayer) {
  for (int i = 0; i < deviceLayer_->getDevicesCount(); ++i) {
    devices_.emplace_back(DeviceId{i});
    queueHelpers_.emplace_back(QueueHelper{deviceLayer->getSubmissionQueuesCount(i)});
  }
  auto dramBaseAddress = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  RT_LOG(INFO) << std::hex << "Runtime initialization. Dram base addr: " << dramBaseAddress
               << " Dram size: " << dramSize;
  for (auto&& d : devices_) {
    memoryManagers_.insert({d, MemoryManager{dramBaseAddress, dramSize, kMinAllocationSize}});
    dmaBufferManagers_.insert({d, std::make_unique<DmaBufferManager>(deviceLayer_, static_cast<int>(d))});
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
  std::unique_lock<std::recursive_mutex> lock(mutex_);

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
      RT_DLOG(INFO) << "Found segment: " << segment->get_index() << std::hex << " Offset: 0x" << offset
                    << " Physical Address: 0x" << loadAddress << " Mem Size: 0x" << memSize << " Copying to address: 0x"
                    << addr << " Entry: 0x" << entry << "\n";
      memcpyHostToDevice(sstream, reinterpret_cast<const uint8_t*>(data) + offset, reinterpret_cast<void*>(addr),
                         memSize);
    }
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
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(kernels_, kernel);

  // free the buffer
  freeDevice(it->second->deviceId_, it->second->deviceBuffer_);

  // and remove the kernel
  kernels_.erase(it);
}

void* RuntimeImp::mallocDevice(DeviceId device, size_t size, int alignment) {
  ScopedProfileEvent profileEvent(Class::MallocDevice, profiler_, device);
  RT_DLOG(INFO) << "Malloc requested device " << std::hex << static_cast<std::underlying_type_t<DeviceId>>(device)
                << " size: " << size << " alignment: " << alignment;
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(memoryManagers_, device);
  // enforce size is multiple of alignment
  size = alignment * ((size + alignment - 1) / alignment);
  return static_cast<void*>(it->second.malloc(size, alignment));
}
void RuntimeImp::freeDevice(DeviceId device, void* buffer) {
  ScopedProfileEvent profileEvent(Class::FreeDevice, profiler_, device);
  RT_DLOG(INFO) << "Free at device: " << static_cast<std::underlying_type_t<DeviceId>>(device)
                << " buffer address: " << std::hex << buffer;
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(memoryManagers_, device);
  it->second.free(buffer);
}

StreamId RuntimeImp::createStream(DeviceId device) {
  ScopedProfileEvent profileEvent(Class::CreateStream, profiler_, device);
  RT_DLOG(INFO) << "Creating stream at device: " << static_cast<std::underlying_type_t<DeviceId>>(device);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto streamId = static_cast<StreamId>(nextStreamId_++);
  auto it = streams_.find(streamId);
  if (it != end(streams_)) {
    throw Exception("Can't create stream");
  }
  streams_.emplace(streamId, Stream{device, queueHelpers_[static_cast<uint32_t>(device)].nextQueue()});
  return streamId;
}

void RuntimeImp::destroyStream(StreamId stream) {
  ScopedProfileEvent profileEvent(Class::DestroyStream, profiler_, stream);
  RT_DLOG(INFO) << "Destroy stream: " << static_cast<std::underlying_type_t<StreamId>>(stream);
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);
  streams_.erase(it);
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const void* h_src, void* d_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_, stream);
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);

  auto evt = eventManager_.getNextId();
  it->second.lastEventId_ = evt;
  auto device = it->second.deviceId_;
  auto vq = it->second.vq_;
  
  //TODO this is using a wrong command; to be fixed when implementing the DMA list: https://esperantotech.atlassian.net/browse/SW-7830
  device_ops_api::device_ops_data_write_cmd_t cmd = {0};
  cmd.dst_device_phy_addr = reinterpret_cast<uint64_t>(d_dst);
  RT_VLOG(LOW) << "MemcpyHostToDevice stream: " << static_cast<std::underlying_type_t<StreamId>>(stream) << std::hex
               << " Host address: " << h_src << " Device address: " << cmd.dst_device_phy_addr << " Size: " << size;

  // first check if its a DmaBuffer
  auto dmaBufferManager = find(dmaBufferManagers_, device)->second.get();
  if (dmaBufferManager->isDmaBuffer(reinterpret_cast<const std::byte*>(h_src), size)) {
    cmd.src_host_virt_addr = cmd.src_host_phy_addr = reinterpret_cast<uint64_t>(h_src);
  } else {
    // if not, allocate a buffer, and stage the memory first into it
    auto tmpBuffer = dmaBufferManager->allocate(size, true);
    cmd.src_host_virt_addr = cmd.src_host_phy_addr = reinterpret_cast<uint64_t>(tmpBuffer->getPtr());

    // TODO: It can be copied in background and return control to the user. Future work.
    auto srcPtr = reinterpret_cast<const std::byte*>(h_src);
    std::copy(srcPtr, srcPtr + size, tmpBuffer->getPtr());

    // TODO: There are more efficient ways of achieving this. Future work.
    auto t = std::thread([this, evt, tmpBuffer = std::move(tmpBuffer)]() mutable {
      // wait till the command is acked
      waitForEvent(evt);
      // when exiting, tmpBuffer will be deallocated
    });
    t.detach();
  }
  cmd.size = size;

  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);  
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  cmd.command_info.cmd_hdr.flags = barrier ? 1 : 0;
  sendCommandMasterMinion(it->second, evt, cmd, lock, true);
  profileEvent.setEventId(evt);
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const void* d_src, void* h_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_, stream);
  RT_VLOG(LOW) << "MemcpyDeviceToHost stream: " << static_cast<std::underlying_type_t<StreamId>>(stream) << std::hex
               << " Host address: " << h_dst << " Device address: " << d_src << " Size: " << size;
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream);

  auto evt = eventManager_.getNextId();
  auto& st = it->second;
  st.lastEventId_ = evt;
  auto device = st.deviceId_;
  auto vq = st.vq_;

  //TODO this is using a wrong command; to be fixed when implementing the DMA list: https://esperantotech.atlassian.net/browse/SW-7830
  device_ops_api::device_ops_data_read_cmd_t cmd = {0};
  cmd.size = size;
  cmd.src_device_phy_addr = reinterpret_cast<uint64_t>(d_src);
  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  cmd.command_info.cmd_hdr.flags = barrier ? 1 : 0;

  // first check if its a DmaBuffer
  auto dmaBufferManager = find(dmaBufferManagers_, device)->second.get();
  if (dmaBufferManager->isDmaBuffer(reinterpret_cast<std::byte*>(h_dst), size)) {
    cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(h_dst);
    sendCommandMasterMinion(st, evt, cmd, lock, true);
  } else {
    // if not, allocate a buffer, and copy the results from it to h_dst
    auto tmpBuffer = dmaBufferManager->allocate(size, false);
    cmd.dst_host_virt_addr = cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(tmpBuffer->getPtr());
    sendCommandMasterMinion(st, evt, cmd, lock, true);

    // TODO: There are many ways to optimize this, future work.
    // replace the event (because we need to do the copy before dispatching the final event to the user)
    auto memcpyEvt = evt;
    evt = eventManager_.getNextId();
    st.lastEventId_ = evt;
    auto t = std::thread([this, memcpyEvt, size, evt, h_dst, tmpBuffer = std::move(tmpBuffer)]() mutable {
      // first wait till the copy ends
      waitForEvent(memcpyEvt);
      // copy results to user buffer
      std::copy(tmpBuffer->getPtr(), tmpBuffer->getPtr() + size, reinterpret_cast<std::byte*>(h_dst));
      // release the dmaBuffer before exiting the thread
      tmpBuffer.reset();
      // dispatch the event
      eventManager_.dispatch(evt);
    });
    t.detach();
  }
  
  profileEvent.setEventId(evt);
  return evt;
}

void RuntimeImp::waitForEvent(EventId event) {
  waitForEvent(event, std::chrono::hours(24)); // we can't use seconds::max
}

void RuntimeImp::waitForStream(StreamId stream) {
  waitForStream(stream, std::chrono::hours(24));
}

bool RuntimeImp::waitForEvent(EventId event, std::chrono::milliseconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForEvent, profiler_, event);
  RT_DLOG(INFO) << "Waiting for event " << static_cast<int>(event) << " to be dispatched.";
  auto res = eventManager_.blockUntilDispatched(event, timeout);
  RT_DLOG(INFO) << "Finished wait for event " << static_cast<int>(event) << " timed out? " << (res ? "false" : "true");
  return res;
}

bool RuntimeImp::waitForStream(StreamId stream, std::chrono::milliseconds timeout) {
  ScopedProfileEvent profileEvent(Class::WaitForStream, profiler_, stream);
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = find(streams_, stream, "Invalid stream");
  auto evt = it->second.lastEventId_;
  lock.unlock();
  RT_DLOG(INFO) << "WaitForStream: Waiting for event " << static_cast<int>(evt);
  return waitForEvent(evt, timeout);
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

    RT_VLOG(HIGH) << std::hex << " Start time: " << response.device_cmd_start_ts
                  << " Wait time: " << response.device_cmd_wait_dur
                  << " Execution time: " << response.device_cmd_execute_dur;
    evt.setDeviceCmdStartTs(response.device_cmd_start_ts);
    evt.setDeviceCmdWaitDur(response.device_cmd_wait_dur);
    evt.setDeviceCmdExecDur(response.device_cmd_execute_dur);
  };

  RT_VLOG(MID) << "Response received eventId: " << std::hex << static_cast<int>(eventId)
               << " Message Id: " << header->rsp_hdr.msg_id;
  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_read_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      char msg[128];
      snprintf(msg, sizeof msg, "Error on DMA read: %d. Tag id: %d", r->status, static_cast<int>(eventId));
      throw Exception(msg);
    }
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_data_write_rsp_t*>(response.data());
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      char msg[128];
      snprintf(msg, sizeof msg, "Error on DMA write: %d. Tag id: %d", r->status, static_cast<int>(eventId));
      throw Exception(msg);
    }
    fillEvent(event, *r);
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    fillEvent(event, *r);
    kernelParametersCache_->releaseBuffer(eventId);
    if (r->status != device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED) {
      char msg[128];
      snprintf(msg, sizeof msg, "Error on kernel launch: %d. Tag id: %d", r->status, static_cast<int>(eventId));
      throw Exception(msg);
    }
    break;
  }
  }
  profiler_.record(event);
  eventManager_.dispatch(eventId);
}

std::unique_ptr<DmaBuffer> RuntimeImp::allocateDmaBuffer(DeviceId device, size_t size, bool writeable) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = find(dmaBufferManagers_, device);
  return it->second->allocate(size, writeable);
}
