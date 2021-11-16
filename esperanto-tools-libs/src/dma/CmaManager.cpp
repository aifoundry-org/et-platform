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
#include "runtime/IDmaBuffer.h"
#include "runtime/IRuntime.h"
#include <mutex>
using namespace rt;
namespace {
constexpr auto kCmaBlockSize = 1024U;
}

CmaManager::CmaManager(IRuntime& runtime, size_t cmaSize)
  : runtime_(runtime)
  , dmaBuffer_(runtime_.allocateDmaBuffer(DeviceId{0}, cmaSize, true))
  , memoryManager_(reinterpret_cast<uint64_t>(dmaBuffer_->getPtr()), dmaBuffer_->getSize(), kCmaBlockSize) {
}

size_t CmaManager::getFreeBytes() const {
  std::lock_guard lock(mutex_);
  return memoryManager_.getFreeContiguousBytes();
}

void CmaManager::free(std::byte* buffer) {
  std::lock_guard lock(mutex_);
  return memoryManager_.free(buffer);
}

std::byte* CmaManager::alloc(size_t size) {
  std::lock_guard lock(mutex_);
  try {
    return memoryManager_.malloc(size, kCmaBlockSize);
  } catch (...) {
    return nullptr;
  }
}