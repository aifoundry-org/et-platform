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
/// * One side (left/front) is used to allocate Constant buffers: Constant
/// buffers are loaded once when we initialize the model on the device, and stay
/// stationary until the model gets unloaded from the device.
/// * The opposite side (right/back) is used to allocate Placeholder buffers.
/// The placeholder buffers are allocated per inference and de-allocated at the
/// end of the inference execution, resulting in more frequent memory
/// fragmentation
///
/// The bidirectinal allocator tries to reduce the internal memory fragmentation
/// of the data region, by separating the buffers based on their lifecycle on
/// the device and trying to keep the "middle" of the data-region free,
class BidirectionalAllocator final : public BaseMemoryAllocator {
public:
  /// @enum direction of the allocation
  enum class AllocDirection : uint8_t {
    None = 0,
    Front, ///< Allocate a buffer, starting search at the front of the free-list
    Back, ///< Allocatee a buffer starting search from the back of the free-list
  };

  BidirectionalAllocator(BufferOffsetTy base, BufferSizeTy size);

  /// @brief Allocate a buffer of type TesnorType and of size bites startng
  /// from the front of the memory region
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @param[in] alignment Size in bytes of the buffer alignment
  /// @returns  Error of the ID of the buffer that was allocated and its aligned
  /// offset
  ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
  mallocFront(BufferType type, BufferSizeTy size,
              BufferSizeTy alignment = MIN_ALIGNMENT);

  /// @brief Allocate a buffer of type TesnorType and of size bites startng
  /// from the front of the memory region
  ///
  /// @param[in] type Type of the buffer to allocate
  /// @param[in] size Size in bytes of the buffer to allocate
  /// @param[in] alignment Size in bytes of the buffer alignment
  /// @returns  Error of the ID of the buffer that was allocated and its aligned
  /// offset
  ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
  mallocBack(BufferType type, BufferSizeTy size,
             BufferSizeTy alignment = MIN_ALIGNMENT);

  /// @brief Deallocate the specific buffer
  ///
  /// @param[in] id ID of the buffer to deallocate
  /// @returns Error os success
  etrtError free(BufferID tid);

  /// @brief Return the total free memory
  BufferSizeTy freeMemory() override;

  /// @brief Run a sanity check and return true on success
  bool sanityCheck() const override;

  /// @brief Print the allocator state
  void printState() override;

  /// @brief Return JSON string with the state of the allocator
  const std::string stateJSON() const override;

  /// @brief Returns true the buffer has been allocated
  bool bufferExists(BufferID tid) const;

private:
  /// Friend class used for our testing
  friend class ::TestBidirectionalAllocator;

  /// Container type holding the information of the allocated buffers of this
  /// memory allocator
  using allocated_buffer_info = std::list<std::shared_ptr<AbstractBufferInfo>>;

  BufferOffsetTy
      base_; ///< Base offset of the memory region this allocator manages
  BufferSizeTy size_; ///< Size of the memory region this allocator mmanages

  std::list<BufferInfo<FreeRegion>> free_list_; ///< List of un-allocated memory

  allocated_buffer_info
      allocated_front_list_; ///< List of allocated buffers from the front of
                             ///< this allocator

  allocated_buffer_info
      allocated_back_list_; ///< List of allocated buffers from the back in this
                            ///< allocator

  /// @brief Find the information of an allocated buffer
  ///
  /// @param[in] tid  ID of the buffer to deallocate
  /// @returns Pointer to the allocated_list_ object or null otherwise
  allocated_buffer_info::value_type findAllocatedBuffer(BufferID tid) const;

  /// @brief Remove buffer from allocated list if it exists, return true on
  /// success
  ///
  /// @param[in] alloc_list Pointer to the allocation list to do the seach in
  /// @param[in] tid ID of the buffer to remove
  /// @returns True if the buffer was found and removed
  ErrorOr<allocated_buffer_info::value_type>
  removeFromAllocatedList(allocated_buffer_info *alloc_list, BufferID tid);

  const BufferOffsetTy endOffset() const { return base_ + size_; }
};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
