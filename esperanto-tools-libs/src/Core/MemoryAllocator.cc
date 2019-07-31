//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryAllocator.h"

// FIXME this header should be removed
#include "../kernels/sys_inc.h"
#include "Support/HelperMacros.h"

#include <cassert>

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

} // namespace device
} // namespace et_runtime
