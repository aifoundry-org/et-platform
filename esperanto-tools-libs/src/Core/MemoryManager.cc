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
#include "et_device.h"

#include <sys/mman.h>
#include <unistd.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

namespace et_runtime {
namespace device {

bool EtMemRegion::isPtrAlloced(const void *ptr) {
  if (alloced_ptrs.count(ptr) > 0) {
    return true;
  }
  for (auto i : alloced_ptrs) {
    if (i.first <= ptr) {
      if ((const char *)i.first + i.second > ptr) {
        return true;
      }
    } else {
      return false;
    }
  }
  return false;
}

void *EtMemRegion::alloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  uintptr_t currBeg = align_up((uintptr_t)region_base, kAlign);

  for (auto it : alloced_ptrs) {
    uintptr_t currEnd = (uintptr_t)it.first;
    uintptr_t nextBeg = align_up((uintptr_t)it.first + it.second, kAlign);

    assert(currEnd >= currBeg);
    if (currEnd - currBeg >= size) {
      void *ptr = (void *)currBeg;
      alloced_ptrs[ptr] = size;
      return ptr;
    }

    currBeg = nextBeg;
  }

  uintptr_t regionEnd = (uintptr_t)region_base + region_size;
  assert(regionEnd >= currBeg);
  if (regionEnd - currBeg >= size) {
    void *ptr = (void *)currBeg;
    alloced_ptrs[ptr] = size;
    return ptr;
  }

  THROW("Can't alloc memory");
}

void EtMemRegion::free(void *ptr) {
  if (ptr == nullptr) {
    return;
  }
  assert(alloced_ptrs.count(ptr));
  alloced_ptrs.erase(ptr);
}

void EtMemRegion::print() {
  printf("start: %p\n", region_base);
  for (auto it : alloced_ptrs) {
    printf("[%p - %p)\n", it.first, (uint8_t *)it.first + it.second);
  }
  printf("end: %p\n", (uint8_t *)region_base + region_size);
}

void MemoryManager::initMemRegions() {
  void *host_base = mmap(nullptr, kHostMemRegionSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(host_base == MAP_FAILED);
  host_mem_region_.reset(new EtMemRegion(host_base, kHostMemRegionSize));

  void *kDevMemBaseAddr = (void *)GLOBAL_MEM_REGION_BASE;
  size_t kDevMemRegionSize = GLOBAL_MEM_REGION_SIZE;
  void *kKernelsDevMemBaseAddr = (void *)EXEC_MEM_REGION_BASE;
  size_t kKernelsDevMemRegionSize = EXEC_MEM_REGION_SIZE;

  void *dev_base = mmap(kDevMemBaseAddr, kDevMemRegionSize, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(dev_base == MAP_FAILED);
  THROW_IF(dev_base != kDevMemBaseAddr, "Cannot allocate dev memory.");
  dev_mem_region_.reset(new EtMemRegion(dev_base, kDevMemRegionSize));

  void *kernels_dev_base =
      mmap(kKernelsDevMemBaseAddr, kKernelsDevMemRegionSize, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(kernels_dev_base == MAP_FAILED);
  THROW_IF(kernels_dev_base != kKernelsDevMemBaseAddr,
           "Cannot allocate kernels dev memory.");
  kernels_dev_mem_region_.reset(
      new EtMemRegion(kernels_dev_base, kKernelsDevMemRegionSize));

  EtActionConfigure *actionConfigure = nullptr;
  EtActionEvent *actionEvent = nullptr;
  {
    std::lock_guard<std::mutex> lk(device_.mutex_);

    assert(device_.defaultStream_ == nullptr);
    device_.defaultStream_ = device_.createStream(false);

    actionConfigure =
        new EtActionConfigure(dev_base, kDevMemRegionSize, kernels_dev_base,
                              kKernelsDevMemRegionSize);
    actionConfigure->incRefCounter();

    actionEvent = new EtActionEvent();
    actionEvent->incRefCounter();

    device_.addAction(device_.defaultStream_, actionConfigure);
    device_.addAction(device_.defaultStream_, actionEvent);
  }

  actionEvent->observerWait();

  if (actionConfigure->isLocalMode()) {
    PERROR_IF(mprotect(dev_base, kDevMemRegionSize, PROT_READ | PROT_WRITE) ==
              -1);
    PERROR_IF(mprotect(kernels_dev_base, kKernelsDevMemRegionSize,
                       PROT_READ | PROT_WRITE) == -1);
  }

  EtAction::decRefCounter(actionConfigure);
  EtAction::decRefCounter(actionEvent);
}

void MemoryManager::uninitMemRegions() {
  munmap(host_mem_region_->region_base, host_mem_region_->region_size);
  host_mem_region_.reset(nullptr);

  munmap(dev_mem_region_->region_base, dev_mem_region_->region_size);
  dev_mem_region_.reset(nullptr);
}

etrtError MemoryManager::mallocHost(void **ptr, size_t size) {
  *ptr = host_mem_region_->alloc(size);
  return etrtSuccess;
}

etrtError MemoryManager::freeHost(void *ptr) {
  host_mem_region_->free(ptr);
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
  if (host_mem_region_->isPtrAlloced(ptr)) {
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

} // namespace device
} // namespace et_runtime
