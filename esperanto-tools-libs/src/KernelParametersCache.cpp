/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "KernelParametersCache.h"
#include "MemoryManager.h"
#include "utils.h"
#include <algorithm>
#include <cassert>
#include <type_traits>
using namespace rt;
using Buffer = KernelParametersCache::Buffer;

Buffer::Buffer(DeviceId device, IRuntime* runtime, size_t size)
  : hostBuffer_(size)
  , device_(device)
  , runtime_(runtime) {
  deviceBuffer_ = runtime->mallocDevice(device, size);
}

Buffer::~Buffer() {
  runtime_->freeDevice(device_, deviceBuffer_);
}

KernelParametersCache::~KernelParametersCache() {
  assert(reservedBuffers_.empty());
  assert(allocBuffers_.empty());
}

KernelParametersCache::KernelParametersCache(IRuntime* runtime, int initialFreeListSize, int bufferSize)
  : runtime_(runtime)
  , bufferSize_(bufferSize) {
  auto devices = runtime->getDevices();
  for (auto dev : devices) {
    auto it = freeBuffers_.emplace(dev, std::vector<Buffer*>{});
    assert(it.second);
    auto&& list = it.first->second;
    for (int i = 0; i < initialFreeListSize; ++i) {
      buffers_.emplace_back(std::make_unique<Buffer>(dev, runtime_, bufferSize_));
      list.emplace_back(buffers_.back().get());
    }
  }
}

Buffer* KernelParametersCache::allocBuffer(DeviceId deviceId) {
  RT_DLOG(INFO) << "Allocating parameter buffer for device " << static_cast<int>(deviceId);
  std::lock_guard lock(mutex_);
  auto&& list = freeBuffers_[deviceId];
  if (list.empty()) {
    buffers_.emplace_back(std::make_unique<Buffer>(deviceId, runtime_, bufferSize_));
    list.emplace_back(buffers_.back().get());
  }
  auto result = list.back();
  [[maybe_unused]] auto [insertion, res] = allocBuffers_.insert(result);
  assert(res);
  list.pop_back();
  return result;
}

void KernelParametersCache::reserveBuffer(EventId event, Buffer* buffer) {
  RT_DLOG(INFO) << "Reserving buffer " << buffer << " for event " << static_cast<int>(event);
  std::lock_guard lock(mutex_);
  auto it = find(allocBuffers_, buffer, "Trying to reserve a buffer which wasn't allocated previously");
  [[maybe_unused]] auto [insertion, res] = reservedBuffers_.emplace(event, *it);
  assert(res);
  allocBuffers_.erase(it);
}

void KernelParametersCache::releaseBuffer(EventId id) {
  RT_DLOG(INFO) << "Releasing buffer for event " << static_cast<int>(id);
  std::lock_guard lock(mutex_);
  auto it = find(reservedBuffers_, id);
  freeBuffers_[it->second->device_].emplace_back(it->second);
  reservedBuffers_.erase(it);
  RT_DLOG(INFO) << "Buffer erased. In use buffers count: " << reservedBuffers_.size();
}
