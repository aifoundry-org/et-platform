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

bool LinearMemoryAllocator::isPtrAllocated(const void *ptr) const {
  uintptr_t tptr = reinterpret_cast<uintptr_t>(ptr);
  for (auto &range : allocated_buffers_) {
    if (range.addr_ <= tptr && tptr < range.end()) {
      return true;
    }
  }
  return false;
}

void *LinearMemoryAllocator::alloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  uintptr_t pointer = 0;

  if (allocated_buffers_.empty()) {
    pointer = align_up(region_base_, kAlign);
  } else {
    auto end = allocated_buffers_.rbegin();
    pointer = align_up(end->end(), kAlign);
  }

  if ((pointer + size) > (region_base_ + region_size_)) {
    RTERROR << "Can't alloc memory \n";
    return nullptr;
  }

  auto res = allocated_buffers_.insert({pointer, size});
  if (!res.second) {
    RTERROR << "Failed to insert memory region \n";
    return nullptr;
  }

  return reinterpret_cast<void *>(pointer);
}

bool LinearMemoryAllocator::emplace(void *ptr, size_t size) {
  if (size == 0) {
    return false;
  }

  uintptr_t tptr = reinterpret_cast<uintptr_t>(ptr);
  auto res = allocated_buffers_.insert({tptr, size});
  return res.second;
}

void LinearMemoryAllocator::free(void *ptr) {
  if (ptr == nullptr) {
    return;
  }
  uintptr_t tptr = reinterpret_cast<uintptr_t>(ptr);
  for (auto it = allocated_buffers_.begin(); it != allocated_buffers_.end();) {
    if (it->addr_ == tptr) {
      allocated_buffers_.erase(it);
      break;
    } else {
      ++it;
    }
  }
}

void LinearMemoryAllocator::print() const {
  printf("start: %p\n", (void *)region_base_);
  for (auto it : allocated_buffers_) {
    printf("[%p - %p)\n", (void *)it.addr_, (void *)it.end());
  }
  printf("end: %p\n", (void *)(region_base_ + region_size_));
}

} // namespace device
} // namespace et_runtime
