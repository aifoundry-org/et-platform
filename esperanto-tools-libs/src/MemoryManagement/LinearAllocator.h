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

#ifndef ET_RUNTIME_MEMORY_MANAGEMENT_LINEAR_ALLOCATOR_H
#define ET_RUNTIME_MEMORY_MANAGEMENT_LINEAR_ALLOCATOR_H

#include "BaseMemoryAllocator.h"

#include <list>
#include <memory>
#include <tuple>

class TestLinearAllocator;

namespace et_runtime {
namespace device {
namespace memory_management {

/// @class LinearAllocator LinearAllocator.h
///
/// @brief LinearALlocator class. Simple memory allocator that does
/// a linear search for the free list and returns first free block big
/// enough to hold the memory block
class LinearAllocator final : public BaseMemoryAllocator {
public:
  LinearAllocator(TensorOffsetTy base, TensorSizeTy size);

  /// @brief Allocate a buffer of type TesnorType and of size bites
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @returns  Error of the ID of the tensor that was allocated
  ErrorOr<TensorID> malloc(TensorType type, TensorSizeTy size);

  /// @brief Deallocate the specific tensor
  ///
  /// @param[in] id ID of the tensor to deallocate
  /// @returns Error os success
  etrtError free(TensorID tid);

  /// @brief Return the total free memory
  TensorSizeTy freeMemory() override;

  /// @brief Print the allocator status
  void printState() override;

  /// @brief Print in the stdout the state of the memory allocator in JSON
  /// format
  void printStateJSON() override;

private:
  /// Friend class used for our testing
  friend class ::TestLinearAllocator;

  /// Container type holding the information of the allocated tensors of this
  /// memory allocator
  using allocated_tensor_info = std::list<std::shared_ptr<AbstractTensorInfo>>;

  TensorOffsetTy base_; ///< Base offset of the memory region this allocator manages
  TensorSizeTy size_; ///< Size of the memory region this allocator mmanages

  std::list<TensorInfo<FreeRegion>> free_list_; ///< List of un-allocated memory
  allocated_tensor_info allocated_list_; ///< List of allocated buffers in this allocator

  /// @brief Find the information of an allocated tensor
  ///
  /// @param[in] tid  ID of the tensor to deallocate
  /// @returns Iterator to the allocated_list_ object
  allocated_tensor_info::iterator findAllocatedTensor(TensorID tid);

};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
