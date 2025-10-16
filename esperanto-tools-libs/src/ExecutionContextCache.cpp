/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "ExecutionContextCache.h"
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include "Utils.h"
#include "runtime/Types.h"

#include <algorithm>
#include <cassert>
#include <type_traits>

using namespace rt;
using Buffer = ExecutionContextCache::Buffer;

Buffer::Buffer(DeviceId device, RuntimeImp* runtime, size_t size)
  : hostBuffer_(size)
  , device_(device)
  , runtime_(runtime) {
  deviceBuffer_ = runtime->doMallocDevice(device, size);
}

Buffer::~Buffer() {
  runtime_->doFreeDevice(device_, deviceBuffer_);
}

ExecutionContextCache::~ExecutionContextCache() {
  RT_LOG_IF(WARNING, !reservedBuffers_.empty() || !allocBuffers_.empty())
    << "Reserved or allocated buffers are not empty in destruction.";
  for (const auto& [event, value] : reservedBuffers_) {
    unused(value);
    RT_LOG(WARNING) << "Reserved buffer for event " << static_cast<int>(event) << " wasn't released.";
  }
  for (auto const buffer : allocBuffers_) {
    RT_LOG(WARNING) << "Allocated buffer 0x" << buffer << " wasn't released.";
  }
  assert(reservedBuffers_.empty());
  assert(allocBuffers_.empty());
}

ExecutionContextCache::ExecutionContextCache(RuntimeImp* runtime, int initialFreeListSize, int bufferSize)
  : runtime_(runtime)
  , bufferSize_(bufferSize) {
  auto devices = runtime_->doGetDevices();
  // TODO: see SW-9219, we fill these buffers with trash until this is properly handled
  std::vector<std::byte> trash;
  std::fill_n(std::back_inserter(trash), bufferSize, std::byte{0xCD});
  for (auto dev : devices) {
    auto st = runtime_->doCreateStream(dev);
    auto [it, res] = freeBuffers_.try_emplace(dev, std::vector<Buffer*>{});
    (void)res;
    assert(res);
    auto& list = it->second;
    for (int i = 0; i < initialFreeListSize; ++i) {
      buffers_.emplace_back(std::make_unique<Buffer>(dev, runtime_, bufferSize_));
      auto bufferPtr = buffers_.back().get();
      // TODO: see SW-9219, we fill these buffers with trash until this is properly handled
      runtime_->doMemcpyHostToDevice(st, trash.data(), bufferPtr->deviceBuffer_, static_cast<size_t>(bufferSize), true,
                                     defaultCmaCopyFunction);
      list.emplace_back(bufferPtr);
    }
    runtime_->doWaitForStream(st);
    runtime_->doDestroyStream(st);
  }
}

Buffer* ExecutionContextCache::allocBuffer(DeviceId deviceId) {
  RT_VLOG(MID) << "Allocating buffer for device " << static_cast<int>(deviceId);
  SpinLock lock(mutex_);
  auto&& list = freeBuffers_[deviceId];
  if (list.empty()) {
    buffers_.emplace_back(std::make_unique<Buffer>(deviceId, runtime_, bufferSize_));
    list.emplace_back(buffers_.back().get());
  }
  auto result = list.back();
  auto [insertion, res] = allocBuffers_.insert(result);
  unused(insertion, res);
  assert(res);
  list.pop_back();
  return result;
}

void ExecutionContextCache::reserveBuffer(EventId event, Buffer* buffer) {
  RT_VLOG(MID) << "Reserving buffer " << buffer << " for event " << static_cast<int>(event);
  SpinLock lock(mutex_);
  auto it = find(allocBuffers_, buffer, "Trying to reserve a buffer which wasn't allocated previously");
  auto [insertion, res] = reservedBuffers_.emplace(event, *it);
  unused(insertion, res);
  assert(res);
  allocBuffers_.erase(it);
}

Buffer* ExecutionContextCache::getReservedBuffer(EventId eventId) const {
  SpinLock lock(mutex_);
  if (auto it = reservedBuffers_.find(eventId); it != end(reservedBuffers_)) {
    return it->second;
  }
  return nullptr;
}

void ExecutionContextCache::releaseBuffer(EventId id) {
  RT_VLOG(MID) << "Releasing buffer for event " << static_cast<int>(id);
  SpinLock lock(mutex_);
  auto it = find(reservedBuffers_, id);
  freeBuffers_[it->second->device_].emplace_back(it->second);
  reservedBuffers_.erase(it);
  RT_VLOG(MID) << "Buffer erased. In use buffers count: " << reservedBuffers_.size();
}
