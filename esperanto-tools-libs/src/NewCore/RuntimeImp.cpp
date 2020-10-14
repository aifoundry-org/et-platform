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
#include "MemoryManager.h"
#include "TargetSilicon.h"
#include "TargetSysEmu.h"
#include <elfio/elfio.hpp>
#include <sstream>

using namespace rt;

namespace {
template <typename Container, typename Key> auto find(Container&& c, Key&& k, std::string error = "Not found") {
  auto it = c.find(k);
  if (it == end(c)) {
    throw Exception(std::move(error));
  }
  return it;
}
}


RuntimeImp::Kernel::Kernel(DeviceId deviceId, std::byte* elfData, size_t elfSize, std::byte* deviceBuffer)
  : deviceId_(deviceId)
  , deviceBuffer_(deviceBuffer) {

  auto memStream = std::istringstream(std::string(reinterpret_cast<char*>(elfData), elfSize), std::ios::binary);
  if (!elf_.load(memStream)) {
    throw Exception("Error parsing elf");
  }
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
    memoryManagers_.insert({d, MemoryManager{target_->getDramBaseAddr(), target_->getDramSize()}});
  }
}

std::vector<DeviceId> RuntimeImp::getDevices() const {
  return target_->getDevices();
}

KernelId RuntimeImp::loadCode(DeviceId device, std::byte* data, size_t size) {

  // allocate a buffer in the device to load the code
  auto deviceBuffer = mallocDevice(device, size);

  // copy the code into the device
  target_->writeDevMemDMA(reinterpret_cast<uint64_t>(deviceBuffer), size, data);
  auto kernel = std::make_unique<Kernel>(device, data, size, deviceBuffer);

  // store the ref
  auto kernelId = static_cast<KernelId>(nextKernelId_++);
  kernels_.insert({kernelId, std::move(kernel)});
  return kernelId;
}

void RuntimeImp::unloadCode(KernelId kernel) {
  auto it = find(kernels_, kernel);
  
  //free the buffer
  freeDevice(it->second->deviceId_, it->second->deviceBuffer_);

  //and remove the kernel
  kernels_.erase(it);
}

std::byte* RuntimeImp::mallocDevice(DeviceId device, size_t size, int alignment) {
  auto it = find(memoryManagers_, device);
  // enforce size is multiple of alignment
  size = alignment * ((size + alignment - 1) / alignment);
  return static_cast<std::byte*>(it->second.malloc(size, alignment));
}
void RuntimeImp::freeDevice(DeviceId device, std::byte* buffer) {
  auto it = find(memoryManagers_, device);
  it->second.free(buffer);
}

StreamId RuntimeImp::createStream(DeviceId device) {
  throw Exception("Not implemented yet");
}
void RuntimeImp::destroyStream(StreamId stream) {
  throw Exception("Not implemented yet");
}

EventId RuntimeImp::kernelLaunch(StreamId stream, KernelId kernel, std::byte* kernel_args, size_t kernel_args_size,
                                 bool barrier) {
  throw Exception("Not implemented yet");
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, std::byte* src, std::byte* dst, size_t size, bool barrier) {
  throw Exception("Not implemented yet");
}
EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, std::byte* src, std::byte* dst, size_t size, bool barrier) {
  throw Exception("Not implemented yet");
}

void RuntimeImp::waitForEvent(EventId event) {
  throw Exception("Not implemented yet");
}
void RuntimeImp::waitForStream(StreamId stream) {
  throw Exception("Not implemented yet");
}