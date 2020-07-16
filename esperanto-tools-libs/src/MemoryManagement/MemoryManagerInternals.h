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

#ifndef ET_RUNTIME_MEMORY_MANAGEMENT_MEMORY_MANAGER_INTERNALS_H
#define ET_RUNTIME_MEMORY_MANAGEMENT_MEMORY_MANAGER_INTERNALS_H

#include "BufferInfo.h"

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>

class TestMemoryManagerInternals;

namespace et_runtime {
namespace device {
namespace memory_management {

class LinearAllocator;
class BidirectionalAllocator;

/// @class MemoryManagerInternals MemoryManagerInternals.h
///
/// Implementation of the memory manager per device. It combines
/// the linear allocator (used for the code-region) and the
/// bidirectional-allocator used for the data-region.
///
/// It expsoes an interface that allocates the different types of
/// buffers, deallocates memory and reports the status of
class MemoryManagerInternals {
public:
  MemoryManagerInternals(uint64_t code_size, uint64_t data_size);
  ~MemoryManagerInternals() = default;

  /// @brief Allocate a buffer in the code region
  ///
  /// @param[in] size Size of the buffer in bytes
  ErrorOr<BufferID> mallocCode(BufferSizeTy size);

  /// @brief Allocate a Constant buffer in the data region
  ///
  /// @param[in] size Size of the buffer in bytes
  ErrorOr<BufferID> mallocConstant(BufferSizeTy size);

  /// @brief Allocate a Placeholder buffer in the data region
  ///
  /// @param[in] size Size of the buffer in bytes
  ErrorOr<BufferID> mallocPlaceholder(BufferSizeTy size);

  /// @brief Free a buffer from the code region
  //
  // @param[in] id ID of the buffer to deallocate from the code region.
  etrtError freeCode(BufferID id);

  /// @brief Free a buffer from the data region
  //
  // @param[in] id ID of the buffer to deallocate from the data region.
  etrtError freeData(BufferID id);

  /// @brief Report the sum of free memory both in the code and data regions.
  uint64_t freeMemory();

  /// @brief Print the state of both the code and data regions
  void printState();

private:
  std::unique_ptr<LinearAllocator> code_region_;
  std::unique_ptr<BidirectionalAllocator> data_region_;
};

} // namespace memory_management
} // namespace device
} // namespace et_runtime

#endif
