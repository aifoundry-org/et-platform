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

#include "esperanto/runtime/Support/MemoryRange.h"

#include <cstdint>
#include <set>

namespace et_runtime {
namespace device {

/// @brief Linear Memory Allocator
///
/// Linear memory allocator that can allocate consecutive memory
/// buffers in a given region. Curretly it does not have the ability
/// to handle any "wholes" in the region created by freeing memory
class LinearMemoryAllocator {
public:
  static constexpr size_t kAlign = 1 << 20; // 1M

  LinearMemoryAllocator(uintptr_t ptr, size_t size)
      : region_base_(ptr), region_size_(size) {}
  /// @brief Return true if pointer is allocated
  bool isPtrAllocated(const void *ptr) const;

  /// @brief Allocate a region of memory
  ///
  /// @params[in] size Size of the buffer to allocate
  /// @return Pointer to allocated region
  void *alloc(size_t size);

  /// @brief "Emplace" a buffer in the region
  ///
  /// Try to insert an allocate buffer. This can be used to exclude
  /// a memory region from being allocated.
  bool emplace(void *ptr, size_t size);

  /// @brief Free an allocated buffer
  void free(void *ptr);

  /// @brief Print the allocated memory regions
  void print() const;

  /// @brief Return true of the pointer is allocated in the regious
  bool isPtrInRegion(const void *ptr) {
    return reinterpret_cast<uintptr_t>(ptr) >= region_base_ &&
           reinterpret_cast<uintptr_t>(ptr) < region_base_ + region_size_;
  }

private:
  uintptr_t region_base_;
  size_t region_size_;
  std::set<support::MemoryRange>
      allocated_buffers_; ///< Set of memory ranges alocated, each range is the
                          ///< buffers pointer and size
};

} // namespace device

} // namespace et_runtime
#endif // ET_RUNTIME_MEMORY_ALLOCATOR_H
