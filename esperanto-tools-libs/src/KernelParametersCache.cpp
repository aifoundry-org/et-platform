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
#include <cassert>
#include <type_traits>
using namespace rt;

KernelParametersCache::~KernelParametersCache() {
  assert(reservedBuffers_.empty());
  assert(allocBuffers_.empty());
}

KernelParametersCache::KernelParametersCache(IRuntime* runtime, int initialFreeListSize, int bufferSize)
  : runtime_(runtime)
  , bufferSize_(bufferSize) {
  auto devices = runtime->getDevices();
  for (auto dev : devices) {
    auto it = freeBuffers_.emplace(dev, std::vector<void*>{});
    assert(it.second);
    auto&& list = it.first->second;
    for (int i = 0; i < initialFreeListSize; ++i) {
      list.emplace_back(runtime->mallocDevice(dev, bufferSize_));
    }
  }
}

void* KernelParametersCache::allocBuffer(DeviceId deviceId) {
  RT_DLOG(INFO) << "Allocating parameter buffer for device " << static_cast<int>(deviceId);
  std::lock_guard lock(mutex_);
  auto&& list = freeBuffers_[deviceId];
  void* result;
  if (list.empty()) {
    result = runtime_->mallocDevice(deviceId, bufferSize_);
  } else {
    result = list.back();
    list.pop_back();
  }
  auto res = allocBuffers_.emplace(InUseBuffer{deviceId, result});
  assert(res.second);
  return result;
}

void KernelParametersCache::reserveBuffer(EventId event, void* buffer) {
  RT_DLOG(INFO) << "Reserving buffer " << buffer << " for event " << static_cast<int>(event);
  std::lock_guard lock(mutex_);
  auto it = find(allocBuffers_, InUseBuffer{DeviceId{}, buffer});
  auto res = reservedBuffers_.emplace(event, *it);
  assert(res.second);
  allocBuffers_.erase(it);
}

void KernelParametersCache::releaseBuffer(EventId id) {
  RT_DLOG(INFO) << "Releasing buffer for event " << static_cast<int>(id);
  std::lock_guard lock(mutex_);
  auto it = find(reservedBuffers_, id);
  freeBuffers_[it->second.device_].emplace_back(it->second.buffer_);
  reservedBuffers_.erase(it);
  RT_DLOG(INFO) << "Buffer erased. In use buffers count: " << reservedBuffers_.size();
}
