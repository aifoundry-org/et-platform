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

CmaManager::CmaManager(std::unique_ptr<IDmaBuffer> dmaBuffer, size_t maxBytesPerCommand)
  : dmaBuffer_(std::move(dmaBuffer))
  , memoryManager_(reinterpret_cast<uint64_t>(dmaBuffer_->getPtr()), dmaBuffer_->getSize(), kCmaBlockSize)
  , maxBytesPerCommand_(maxBytesPerCommand) {
  RT_VLOG(LOW) << "Runtime CMA allocation size: 0x" << std::hex << dmaBuffer_->getSize();
}

void CmaManager::addMemcpyAction(std::unique_ptr<actionList::IAction> action) {
  memcpyActionManager_.addAction(std::move(action));
}

size_t CmaManager::getTotalSize() const {
  return dmaBuffer_->getSize();
}

size_t CmaManager::getFreeBytes() const {
  SpinLock lock(mutex_);
  auto freeBytes = memoryManager_.getFreeContiguousBytes();
  RT_VLOG(MID) << "Free CMA bytes: " << freeBytes;
  return freeBytes;
}

void CmaManager::free(std::byte* buffer) {
  SpinLock lock(mutex_);
  memoryManager_.free(buffer);
  // we have more memory available so greedily wakeup the runner
  memcpyActionManager_.update();
}

std::byte* CmaManager::alloc(size_t size) {
  RT_VLOG(MID) << "Trying to allocate: " << size << " bytes of CMA memory";
  SpinLock lock(mutex_);
  try {
    return memoryManager_.malloc(size, kCmaBlockSize);
  } catch (...) {
    RT_VLOG(MID) << "CMA allocation failed; not enough memory.";
    return nullptr;
  }
}