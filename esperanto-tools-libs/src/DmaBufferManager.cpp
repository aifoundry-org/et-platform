/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "DmaBufferManager.h"
#include "runtime/DmaBuffer.h"
#include <algorithm>
using namespace rt;

bool DmaBufferManager::isDmaBuffer(std::byte* address, size_t size) const {
  if (inUseBuffers_.empty())
    return false;
  auto it = std::upper_bound(begin(inUseBuffers_), end(inUseBuffers_), address);
  if (it == begin(inUseBuffers_))
    return false;
  return (it - 1)->containsAddr(address);
}

std::unique_ptr<DmaBuffer> DmaBufferManager::allocate(size_t size, bool writeable) {
  auto bufferImp = std::make_unique<DmaBufferImp>(device_, size, writeable, deviceLayer_);
  auto it = std::upper_bound(begin(inUseBuffers_), end(inUseBuffers_), *bufferImp);
  inUseBuffers_.emplace(it, *bufferImp);
  return std::make_unique<DmaBuffer>(std::move(bufferImp), device_, this);
}

void DmaBufferManager::release(DmaBufferImp&& dmaBuffer) {
  deviceLayer_->freeDmaBuffer(dmaBuffer.address_);
}