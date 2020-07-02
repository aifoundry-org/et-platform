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

#ifndef ET_RUNTIME_MEMORY_MANAGEMENT_BIDIRECTIONAL_ALLOCATOR_H
#define ET_RUNTIME_MEMORY_MANAGEMENT_BIDIRECTIONAL_ALLOCATOR_H

#include "BaseMemoryAllocator.h"

#include <list>
#include <memory>
#include <tuple>

class TestBidirectionalAllocator;

namespace et_runtime {
namespace device {
namespace memory_management {

/// @class BidirectionalAllocator BidirectionalAllocator.h
///
/// @brief BidirectionalALlocator class. Memory allocator that grows two
/// memory regions internally similar to a "heap" and a "stack" of a real system

/// The bidirectional memory allocator can handle memory allocation requests
/// from "both" ends of the memory region (left, or right). Regardless of the
/// starting point, it will start doing a linear search to try to find the first
/// free region big enough the hold the requested memory. The flow is similar to
/// having a "heap" and a "stack" that keep growing from the different ends of
/// the memory region.
///
/// The bidirectional allocator will be used to manage the data-region of the
/// device's DRAM:
///
/// * One side (left/front) is used to allocate Constant tensors: Constant
/// tensors are loaded once when we initialize the model on the device, and stay
/// stationary until the model gets unloaded from the device.
/// * The opposite side (right/back) is used to allocate Placeholder tensors.
/// The placeholder tensors are allocated per inference and de-allocated at the
/// end of the inference execution, resulting in more frequent memory
/// fragmentation
///
/// The bidirectinal allocator tries to reduce the internal memory fragmentation
/// of the data region, by separating the tensors based on their lifecycle on
/// the device and trying to keep the "middle" of the data-region free,
class BidirectionalAllocator final : public BaseMemoryAllocator {
public:
  /// @enum direction of the allocation
  enum class AllocDirection : uint8_t {
    None = 0,
    Front, ///< Allocate a tensor, starting search at the front of the free-list
    Back, ///< Allocatee a tensor starting search from the back of the free-list
  };

  BidirectionalAllocator(TensorOffsetTy base, TensorSizeTy size);

  /// @brief Allocate a buffer of type TesnorType and of size bites startng
  /// from the front of the memory region
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @returns  Error of the ID of the tensor that was allocated
  ErrorOr<TensorID> mallocFront(TensorType type, TensorSizeTy size);

  /// @brief Allocate a buffer of type TesnorType and of size bites startng
  /// from the front of the memory region
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @returns  Error of the ID of the tensor that was allocated
  ErrorOr<TensorID> mallocBack(TensorType type, TensorSizeTy size);

  /// @brief Deallocate the specific tensor
  ///
  /// @param[in] id ID of the tensor to deallocate
  /// @returns Error os success
  etrtError free(TensorID tid);

  /// @brief Return the total free memory
  TensorSizeTy freeMemory() override;

  /// @brief Print the allocator status
  void printState() override;

private:
  /// Friend class used for our testing
  friend class ::TestBidirectionalAllocator;

  /// Container type holding the information of the allocated tensors of this
  /// memory allocator
  using allocated_tensor_info = std::list<std::shared_ptr<AbstractTensorInfo>>;

  TensorOffsetTy base_; ///< Base offset of the memory region this allocator manages
  TensorSizeTy size_; ///< Size of the memory region this allocator mmanages

  std::list<TensorInfo<FreeRegion>> free_list_; ///< List of un-allocated memory

  allocated_tensor_info
      allocated_front_list_; ///< List of allocated buffers from the front of
                             ///< this allocator

  allocated_tensor_info
      allocated_back_list_; ///< List of allocated buffers from the back in this
                            ///< allocator

  /// @brief Find the information of an allocated tensor
  ///
  /// @param[in] tid  ID of the tensor to deallocate
  /// @returns Iterator to the allocated_list_ object
  allocated_tensor_info::value_type findAllocatedTensor(TensorID tid);

  /// @brief Remove tensor from allocated list if it exists, return true on
  /// success
  ///
  /// @param[in] alloc_list Pointer to the allocation list to do the seach in
  /// @param[in] tid ID of the tensor to remove
  /// @returns True if the tensor was found and removed
  ErrorOr<allocated_tensor_info::value_type>
  removeFromAllocatedList(allocated_tensor_info *alloc_list, TensorID tid);
};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
