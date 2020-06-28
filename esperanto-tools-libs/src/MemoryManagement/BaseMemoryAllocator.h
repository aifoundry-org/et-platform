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

#include "TensorInfo.h"

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

  /// @brief Allocate a buffer of type TesnorType and of size bites
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @returns  Error of the ID of the tensor that was allocated
  virtual ErrorOr<TensorID> malloc(TensorType type, TensorSizeTy size) = 0;

  /// @brief Deallocate the specific tensor
  ///
  /// @param[in] id ID of the tensor to deallocate
  /// @returns Error os success
  virtual etrtError free(TensorID) = 0;

  /// @brief Return the total free memory
  virtual TensorSizeTy freeMemory() = 0;

  /// @brief Print in the stdout the state of the memory allocator
  virtual void printState() = 0;

  /// @brief Return the meta-data size of a specific tensor type
  static TensorSizeTy mdSize(TensorType type);

  /// @brief Create a tensor of a specific type.
  ///
  /// @param[in] type Type of the tensor
  /// @param[in] base Base offset of the tensor in the memory region
  /// @param[in] size Size in bytes of the tensor/memory-region
  static std::shared_ptr<AbstractTensorInfo>
  createTensorInfo(TensorType type, TensorOffsetTy base, TensorSizeTy size);

private:
};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
