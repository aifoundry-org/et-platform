//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/MemoryManager.h"

#include "DeviceAPI/Commands.h"
#include "MemoryManagement/BidirectionalAllocator.h"
#include "MemoryManagement/LinearAllocator.h"
#include "MemoryManagement/MemoryManagerInternals.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/EsperantoRuntime.h"

#include <esperanto-fw/fw-helpers/layout.h>
#include <sys/mman.h>
#include <unistd.h>

namespace et_runtime {
namespace device {

MemoryManager::MemoryManager(Device &dev)
    : impl_(), device_(dev), data_buffer_map_(), code_buffer_map_() {}

MemoryManager::~MemoryManager() {}

bool MemoryManager::init() {
  initMemRegions();
  return true;
}

bool MemoryManager::deInit() {
  uninitMemRegions();
  return true;
}

uintptr_t MemoryManager::ramBase() const { return device_.dramBaseAddr(); }

void MemoryManager::initMemRegions() {
  assert(MemoryManager::CODE_SIZE < device_.dramSize());

  impl_.reset(new memory_management::MemoryManagerInternals(
      device_.dramBaseAddr(), MemoryManager::CODE_SIZE,
      device_.dramSize() - MemoryManager::CODE_SIZE));
}

void MemoryManager::uninitMemRegions() {

}

etrtError MemoryManager::reserveMemoryCode(uintptr_t ptr, size_t size) {
  auto res = impl_->emplaceCode(ptr, size);
  if (!res) {
    return etrtErrorHostMemoryAlreadyRegistered;
  }
  return etrtSuccess;
}

etrtError MemoryManager::malloc(void **devPtr, size_t size) {
  auto res = impl_->mallocConstant(size, MemoryManager::DATA_ALIGNMENT);
  if (!res) {
    return res.getError();
  }
  auto [buffer_id, base] = res.get();
  *devPtr = reinterpret_cast<void *>(base);
  data_buffer_map_[base] = buffer_id;
  TRACE_MemoryManager_malloc(size, reinterpret_cast<uint64_t>(*devPtr));
  return etrtSuccess;
}

etrtError MemoryManager::mallocCode(void **devPtr, size_t size) {
  auto res = impl_->mallocCode(size, DATA_ALIGNMENT);
  if (!res) {
    return res.getError();
  }
  auto [buffer_id, base] = res.get();
  *devPtr = reinterpret_cast<void *>(base);
  code_buffer_map_[base] = buffer_id;
  TRACE_MemoryManager_malloc(size, reinterpret_cast<uint64_t>(*devPtr));
  return etrtSuccess;
}

etrtError MemoryManager::free(void *devPtr) {
  auto base = reinterpret_cast<BufferOffsetTy>(devPtr);
  auto it = data_buffer_map_.find(base);
  assert(it != data_buffer_map_.end());
  auto bid = it->second;
  data_buffer_map_.erase(it);
  return impl_->freeData(bid);
}

etrtError MemoryManager::freeCode(void *devPtr) {
  auto base = reinterpret_cast<BufferOffsetTy>(devPtr);
  auto it = code_buffer_map_.find(base);
  assert(it != code_buffer_map_.end());
  auto bid = it->second;
  code_buffer_map_.erase(it);
  return impl_->freeCode(bid);
}

bool MemoryManager::isPtrInDevRegion(const void *ptr) {
  return impl_->dataBufferExists(reinterpret_cast<BufferID>(ptr));
}

bool MemoryManager::runSanityCheck() const { return impl_->runSanityCheck(); }

void MemoryManager::recordState() const { impl_->recordState(); }

} // namespace device
} // namespace et_runtime
