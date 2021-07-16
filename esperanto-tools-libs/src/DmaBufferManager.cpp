/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "DmaBufferManager.h"
#include "runtime/DmaBuffer.h"
#include "utils.h"
#include <algorithm>
using namespace rt;

bool DmaBufferManager::isDmaBuffer(const std::byte* address) const {
  std::lock_guard lock(mutex_);
  if (inUseBuffers_.empty())
    return false;
  // this tmp is just because I don't want to specialize the less operator for checking against address instead of
  // DmaBuffers
  auto tmp = DmaBufferImp{};
  tmp.address_ = const_cast<decltype(tmp.address_)>(address);
  auto it = std::lower_bound(begin(inUseBuffers_), end(inUseBuffers_), tmp);
  if (it == end(inUseBuffers_))
    return false;
  return it->containsAddr(address);
}

DmaBuffer DmaBufferManager::allocate(size_t size, bool writeable) {
  std::lock_guard lock(mutex_);
  auto bufferImp = std::make_unique<DmaBufferImp>(device_, size, writeable, deviceLayer_);
  auto it = std::upper_bound(begin(inUseBuffers_), end(inUseBuffers_), *bufferImp);
  inUseBuffers_.emplace(it, *bufferImp);
  return DmaBuffer{std::move(bufferImp), this};
}

void DmaBufferManager::release(DmaBufferImp* dmaBuffer) {
  std::lock_guard lock(mutex_);
  // TODO: in the future we will use a pool
  deviceLayer_->freeDmaBuffer(dmaBuffer->address_);
  auto it = std::lower_bound(begin(inUseBuffers_), end(inUseBuffers_), *dmaBuffer);
  if (it == end(inUseBuffers_) || it->address_ != dmaBuffer->address_) {
    throw Exception("Trying to release a DmaBuffer that wasn't previously allocated");
  }
  inUseBuffers_.erase(it);
}