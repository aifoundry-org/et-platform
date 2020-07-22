//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

/// @file

#ifndef ET_RUNTIME_MEMORY_MANAGEMENT_BASE_MEMORY_ALLOCATOR_H
#define ET_RUNTIME_MEMORY_MANAGEMENT_BASE_MEMORY_ALLOCATOR_H

#include "BufferInfo.h"

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>

namespace et_runtime {
namespace device {
namespace memory_management {

/// @class BaseMemoryAllocator BaseMemoryAllocator.h
///
/// @brief Base abstract class that provides the interface of a memory
/// allocator over a specific memory range
class BaseMemoryAllocator {
public:
  BaseMemoryAllocator() = default;

  /// @brief Do not allow for a memory allocator to be copied
  BaseMemoryAllocator(BaseMemoryAllocator &) = delete;
  virtual ~BaseMemoryAllocator() = default;

  /// @brief Deallocate the specific buffer
  ///
  /// @param[in] id ID of the buffer to deallocate
  /// @returns Error os success
  virtual etrtError free(BufferID) = 0;

  /// @brief Return the total free memory
  virtual BufferSizeTy freeMemory() = 0;

  /// @brief Print in the stdout the state of the memory allocator
  virtual void printState() = 0;

  /// @brief Print in the stdout the state of the memory allocator in JSON
  /// format
  virtual void printStateJSON() = 0;

  /// @brief Return the meta-data size of a specific buffer type
  static BufferSizeTy mdSize(BufferType type);

  /// @brief Create a buffer of a specific type.
  ///
  /// @param[in] type Type of the buffer
  /// @param[in] base Base offset of the buffer in the memory region
  /// @param[in] aligned_start Aligned offset that is the actuall start of the
  /// buffer we report to the user
  /// @param[in] req_size Requested allocation size
  /// @param[in] size Size in bytes of the buffer/memory-region
  static std::shared_ptr<AbstractBufferInfo>
  createBufferInfo(BufferType type, BufferOffsetTy base,
                   BufferOffsetTy aligned_start, BufferSizeTy req_size,
                   BufferSizeTy size);

protected:
  /// @brief Increase the size of the allocation to accomodate for returning
  /// an aligned pointer
  static BufferSizeTy alignmentFixSize(BufferSizeTy size, BufferSizeTy mdSize,
                                       BufferSizeTy alignment);

  /// @brief Return an alinged stating offset that will be the return value of
  /// the memory allocation
  static BufferSizeTy alignedStart(BufferOffsetTy base, BufferSizeTy mdSize,
                                   BufferSizeTy alignment);
};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
