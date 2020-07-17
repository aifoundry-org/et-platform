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

#include "Core/MemoryAllocator.h"
#include "DeviceAPI/Commands.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/EsperantoRuntime.h"

#include <sys/mman.h>
#include <unistd.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

namespace et_runtime {
namespace device {

MemoryManager::MemoryManager(Device &dev) : device_(dev) {}

MemoryManager::~MemoryManager() {}

bool MemoryManager::init() {
  initMemRegions();
  return true;
}

bool MemoryManager::deInit() {
  uninitMemRegions();
  return true;
}

uintptr_t MemoryManager::ramBase() const { return RAM_MEMORY_REGION; }

void MemoryManager::initMemRegions() {
  dev_mem_region_.reset(new LinearMemoryAllocator(GLOBAL_MEM_REGION_BASE,
                                                  GLOBAL_MEM_REGION_SIZE));
  kernels_dev_mem_region_.reset(
    new LinearMemoryAllocator(EXEC_MEM_REGION_BASE, EXEC_MEM_REGION_SIZE));

}

void MemoryManager::uninitMemRegions() {

}

etrtError MemoryManager::reserveMemory(void *ptr, size_t size) {
  // For now this applies only to the dev mem region
  auto res = dev_mem_region_->emplace(ptr, size);
  if (!res) {
    return etrtErrorHostMemoryAlreadyRegistered;
  }
  return etrtSuccess;
}

etrtError MemoryManager::malloc(void **devPtr, size_t size) {
  *devPtr = dev_mem_region_->alloc(size);
  TRACE_MemoryManager_malloc(size, reinterpret_cast<uint64_t>(*devPtr));
  return etrtSuccess;
}

etrtError MemoryManager::free(void *devPtr) {
  dev_mem_region_->free(devPtr);
  return etrtSuccess;
}

bool MemoryManager::isPtrAllocatedDev(const void *ptr) {
  return dev_mem_region_->isPtrAllocated(ptr);
}

bool MemoryManager::isPtrInDevRegion(const void *ptr) {
  return dev_mem_region_->isPtrInRegion(ptr);
}

} // namespace device
} // namespace et_runtime
