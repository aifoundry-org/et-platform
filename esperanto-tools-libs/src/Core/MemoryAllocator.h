//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_ALLOCATOR_H
#define ET_RUNTIME_MEMORY_ALLOCATOR_H

#include <cstdint>
#include <map>

namespace et_runtime {
namespace device {

/// @brief Struct holding the beging and size of an allocated buffer
///
/// This struct will be used to indentify the allocated memory regions
/// It will provide
struct MemoryRange {
  MemoryRange() = default;
  // The less operator will return true iff this range is not overlapping
  // and to the left of the "other" one
  bool operator<(const MemoryRange other) const {
    return (addr_ + size_) <= other.addr_;
  }
  uintptr_t addr_ = 0;
  size_t size_ = 0;
};

/// @brief Linear Memory Allocator
///
/// Linear memory allocator that can allocate consecutive memory
/// buffers in a given region. Curretly it does not have the ability
/// to handle any "wholes" in the region created by freeing memory
struct LinearMemoryAllocator {
  void *region_base;
  size_t region_size;
  std::map<const void *, size_t>
      alloced_ptrs; // alloced ptr -> size of alloced area

  LinearMemoryAllocator(void *ptr, size_t size) : region_base(ptr), region_size(size) {}

  static constexpr size_t kAlign = 1 << 20; // 1M
  bool isPtrAlloced(const void *ptr);
  void *alloc(size_t size);
  void free(void *ptr);
  void print();
  bool isPtrInRegion(const void *ptr) {
    return ptr >= region_base &&
           (uintptr_t)ptr < (uintptr_t)region_base + region_size;
  }
};

} // namespace device

} // namespace et_runtime
#endif // ET_RUNTIME_MEMORY_ALLOCATOR_H
