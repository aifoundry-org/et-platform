/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "CmaManager.h"
#include "MemoryManager.h"
#include "Utils.h"
#include "dma/IDmaBuffer.h"
#include "runtime/IRuntime.h"
#include <mutex>
using namespace rt;
namespace {
constexpr auto kCmaBlockSize = 1024U;
}

CmaManager::CmaManager(std::unique_ptr<IDmaBuffer> dmaBuffer)
  : dmaBuffer_(std::move(dmaBuffer))
  , memoryManager_(reinterpret_cast<uint64_t>(dmaBuffer_->getPtr()), dmaBuffer_->getSize(), kCmaBlockSize) {
  RT_VLOG(LOW) << "Runtime CMA allocation size: 0x" << std::hex << dmaBuffer_->getSize();
}

size_t CmaManager::getTotalSize() const {
  return dmaBuffer_->getSize();
}

size_t CmaManager::getFreeBytes() const {
  SpinLock lock(mutex_);
  auto freeBytes = memoryManager_.getFreeContiguousBytes();
  RT_VLOG(HIGH) << "Free CMA bytes: " << freeBytes;
  return freeBytes;
}

void CmaManager::free(std::byte* buffer) {
  std::lock_guard lock(mutex_);
  memoryManager_.free(buffer);
  cv_.notify_all();
}

void CmaManager::waitUntilFree(size_t size) {
  SpinLock lock(mutex_);
  cv_.wait(lock, [this, size] { return size <= memoryManager_.getFreeContiguousBytes(); });
}

std::byte* CmaManager::alloc(size_t size) {
  RT_VLOG(HIGH) << "Trying to allocate: " << size << " bytes of CMA memory.";
  std::lock_guard lock(mutex_);
  try {
    return memoryManager_.malloc(size, kCmaBlockSize);
  } catch (...) {
    RT_VLOG(HIGH) << "CMA allocation failed";
    return nullptr;
  }
}