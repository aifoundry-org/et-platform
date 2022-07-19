/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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