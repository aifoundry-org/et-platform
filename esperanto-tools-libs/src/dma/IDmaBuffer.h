/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once

#include <cstddef>
#include <memory>

namespace rt {
/// \brief This class wraps a buffer which is suitable to perform a Dma operation without having to stage the memory in
/// an intermediate kernel buffer; thus enabling the zero-copy
class IDmaBuffer {
public:
  /// \brief Returns the associated memory pointer where the user can read or write to.
  ///
  /// @returns a pointer to the allocated memory
  ///
  virtual std::byte* getPtr() const = 0;

  /// \brief Returns the size of the allocated memory.
  ///
  /// @returns size of the allocated memory.
  ///
  virtual size_t getSize() const = 0;

  /// \brief virtual dtor
  virtual ~IDmaBuffer() = default;
};

} // namespace rt