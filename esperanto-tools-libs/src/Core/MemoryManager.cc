//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/MemoryManager.h"

#include "Core/MemoryAllocator.h"
#include "DeviceAPI/Commands.h"
#include "EsperantoRuntime.h"

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

void MemoryManager::initMemRegions() {
  void *kDevMemBaseAddr = (void *)GLOBAL_MEM_REGION_BASE;
  size_t kDevMemRegionSize = GLOBAL_MEM_REGION_SIZE;
  void *kKernelsDevMemBaseAddr = (void *)EXEC_MEM_REGION_BASE;
  size_t kKernelsDevMemRegionSize = EXEC_MEM_REGION_SIZE;

  void *dev_base = mmap(kDevMemBaseAddr, kDevMemRegionSize, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(dev_base == MAP_FAILED);
  THROW_IF(dev_base != kDevMemBaseAddr, "Cannot allocate dev memory.");
  dev_mem_region_.reset(new LinearMemoryAllocator(dev_base, kDevMemRegionSize));

  void *kernels_dev_base =
      mmap(kKernelsDevMemBaseAddr, kKernelsDevMemRegionSize, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(kernels_dev_base == MAP_FAILED);
  THROW_IF(kernels_dev_base != kKernelsDevMemBaseAddr,
           "Cannot allocate kernels dev memory.");
  kernels_dev_mem_region_.reset(
      new LinearMemoryAllocator(kernels_dev_base, kKernelsDevMemRegionSize));

  std::shared_ptr<EtAction> actionConfigure = nullptr;
  std::shared_ptr<EtAction> actionEvent = nullptr;

  assert(device_.defaultStream_ == nullptr);
  device_.defaultStream_ = device_.createStream(false);

  auto configure_command = std::make_shared<device_api::ConfigureCommand>(
      dev_base, kDevMemRegionSize, kernels_dev_base, kKernelsDevMemRegionSize);

  device_.addCommand(
      device_.defaultStream_,
      std::dynamic_pointer_cast<device_api::CommandBase>(configure_command));

  auto configure_future = configure_command->getFuture();
  auto configure_resp = configure_future.get();

  if (configure_command->isLocalMode()) {
    PERROR_IF(mprotect(dev_base, kDevMemRegionSize, PROT_READ | PROT_WRITE) ==
              -1);
    PERROR_IF(mprotect(kernels_dev_base, kKernelsDevMemRegionSize,
                       PROT_READ | PROT_WRITE) == -1);
  }
}

void MemoryManager::uninitMemRegions() {

  munmap(dev_mem_region_->region_base, dev_mem_region_->region_size);
  dev_mem_region_.reset(nullptr);

  munmap(kernels_dev_mem_region_->region_base, kernels_dev_mem_region_->region_size);
  kernels_dev_mem_region_.reset(nullptr);
}

etrtError MemoryManager::mallocHost(void **ptr, size_t size) {
  uint8_t *tptr = new uint8_t[size];
  *ptr = tptr;
  host_mem_region_[tptr] = std::unique_ptr<uint8_t>(tptr);
  return etrtSuccess;
}

etrtError MemoryManager::freeHost(void *ptr) {
  uint8_t *tptr = reinterpret_cast<uint8_t *>(ptr);
  auto num_removed = host_mem_region_.erase(tptr);
  if (num_removed == 0) {
    return etrtErrorHostMemoryNotRegistered;
  }
  return etrtSuccess;
}

etrtError MemoryManager::malloc(void **devPtr, size_t size) {
  *devPtr = dev_mem_region_->alloc(size);
  return etrtSuccess;
}

etrtError MemoryManager::free(void *devPtr) {
  dev_mem_region_->free(devPtr);
  return etrtSuccess;
}

etrtError
MemoryManager::pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                    const void *ptr) {
  void *p = (void *)ptr;
  attributes->device = 0;
  attributes->isManaged = false;
  auto tptr = reinterpret_cast<uint8_t *>(const_cast<void *>(ptr));
  if (host_mem_region_.find(tptr) != host_mem_region_.end()) {
    attributes->memoryType = etrtMemoryTypeHost;
    attributes->devicePointer = nullptr;
    attributes->hostPointer = p;
  } else if (dev_mem_region_->isPtrAlloced(ptr)) {
    attributes->memoryType = etrtMemoryTypeDevice;
    attributes->devicePointer = p;
    attributes->hostPointer = nullptr;
  } else {
    THROW("Unexpected pointer");
  }
  return etrtSuccess;
}

bool MemoryManager::isPtrAllocedHost(const void *ptr) {
  uint8_t *tptr = reinterpret_cast<uint8_t *>(const_cast<void *>(ptr));
  return host_mem_region_.find(tptr) == host_mem_region_.end();
}

bool MemoryManager::isPtrAllocedDev(const void *ptr) {
  return dev_mem_region_->isPtrAlloced(ptr);
}

bool MemoryManager::isPtrInDevRegion(const void *ptr) {
  return dev_mem_region_->isPtrInRegion(ptr);
}

} // namespace device
} // namespace et_runtime
