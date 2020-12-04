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
#include "MailboxReader.h"
#include "ScopedProfileEvent.h"
#include "TargetSilicon.h"
#include "TargetSysEmu.h"
#include "utils.h"
#include <cstdio>
#include <elfio/elfio.hpp>
#include <memory>
#include <sstream>
using namespace rt;
using namespace rt::profiling;

RuntimeImp::~RuntimeImp() {
  mailboxReader_.reset();
}

RuntimeImp::RuntimeImp(Kind kind) {
  switch (kind) {
  case Kind::SysEmu:
    target_ = std::make_unique<TargetSysEmu>();
    break;
  case Kind::Silicon:
    target_ = std::make_unique<TargetSilicon>();
    break;
  default:
    throw Exception("Not implemented");
  }
  devices_ = target_->getDevices();
  for (auto&& d : devices_) {
    memoryManagers_.insert({d, MemoryManager{target_->getDramBaseAddr(), target_->getDramSize(), kMinAllocationSize}});
  }
  kernelParametersCache_ = std::make_unique<KernelParametersCache>(this);
  mailboxReader_ = std::make_unique<MailboxReader>(target_.get(), kernelParametersCache_.get(), &eventManager_);
}

std::vector<DeviceId> RuntimeImp::getDevices() {
  ScopedProfileEvent profileEvent(Class::GetDevices, profiler_);
  return target_->getDevices();
}

KernelId RuntimeImp::loadCode(DeviceId device, const void* data, size_t size) {
  ScopedProfileEvent profileEvent(Class::LoadCode, profiler_);

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
      target_->writeDevMemDMA(addr, memSize, reinterpret_cast<const uint8_t*>(data) + offset);
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
  return kernelId;
}

void RuntimeImp::unloadCode(KernelId kernel) {
  ScopedProfileEvent profileEvent(Class::UnloadCode, profiler_);
  auto it = find(kernels_, kernel);

  // free the buffer
  freeDevice(it->second->deviceId_, it->second->deviceBuffer_);

  // and remove the kernel
  kernels_.erase(it);
}

void* RuntimeImp::mallocDevice(DeviceId device, size_t size, int alignment) {
  ScopedProfileEvent profileEvent(Class::MallocDevice, profiler_);
  auto it = find(memoryManagers_, device);
  // enforce size is multiple of alignment
  size = alignment * ((size + alignment - 1) / alignment);
  return static_cast<void*>(it->second.malloc(size, alignment));
}
void RuntimeImp::freeDevice(DeviceId device, void* buffer) {
  ScopedProfileEvent profileEvent(Class::FreeDevice, profiler_);
  auto it = find(memoryManagers_, device);
  it->second.free(buffer);
}

StreamId RuntimeImp::createStream(DeviceId device) {
  ScopedProfileEvent profileEvent(Class::CreateStream, profiler_);
  auto streamId = static_cast<StreamId>(nextStreamId_++);
  auto it = streams_.find(streamId);
  if (it != end(streams_)) {
    throw Exception("Can't create stream");
  }
  streams_.emplace(streamId, Stream{device, 0});
  return streamId;
}

void RuntimeImp::destroyStream(StreamId stream) {
  ScopedProfileEvent profileEvent(Class::DestroyStream, profiler_);
  auto it = find(streams_, stream);
  streams_.erase(it);
}

//#TODO this won't be complete nor real till VQs are implemented information see epic SW-4377
// currently we only create an event, don't
EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const void* h_src, void* d_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_);
  if (size % 256 != 0) { // #TODO fix this with SW-5098
    throw Exception("Memcpy operations must be aligned to 256B");
  }
  auto it = find(streams_, stream);
  auto evt = eventManager_.getNextId();
  it->second.lastEventId_ = evt;
  auto ret = target_->writeDevMemDMA(reinterpret_cast<uint64_t>(d_dst), size, h_src);
  eventManager_.dispatch(evt);
  if (!ret) {
    throw Exception("Error DMA (reading)");
  }
  return evt;
}
//#TODO this won't be complete nor real till VQs are implemented information see epic SW-4377
// currently we only create an event, don't
EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const void* d_src, void* h_dst, size_t size,
                                       [[maybe_unused]] bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_);
  if (size % 256 != 0) { // #TODO fix this with SW-5098
    throw Exception("Memcpy operations must be aligned to 256B");
  }
  auto it = find(streams_, stream);
  auto evt = eventManager_.getNextId();
  it->second.lastEventId_ = evt;
  auto ret = target_->readDevMemDMA(reinterpret_cast<uint64_t>(d_src), size, h_dst);
  eventManager_.dispatch(evt);
  if (!ret) {
    throw Exception("Error DMA (writing)");
  }
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
  auto it = find(streams_, stream, "Invalid stream");
  waitForEvent(it->second.lastEventId_);
}