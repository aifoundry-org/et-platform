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
  RT_LOG(INFO) << "Runtime CMA allocation size: 0x" << std::hex << cmaSize;
}

size_t CmaManager::getFreeBytes() const {
  std::lock_guard lock(mutex_);
  auto freeBytes = memoryManager_.getFreeContiguousBytes();
  RT_VLOG(HIGH) << "Free CMA bytes: " << freeBytes;
  return freeBytes;
}

void CmaManager::free(std::byte* buffer) {
  std::lock_guard lock(mutex_);
  memoryManager_.free(buffer);
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