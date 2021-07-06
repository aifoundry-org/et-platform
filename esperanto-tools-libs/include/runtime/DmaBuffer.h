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
class DmaBufferManager;
struct DmaBufferImp;

class DmaBuffer {

public:
  const std::byte* getPtr() const;
  std::byte* getPtr();
  bool containsAddr(const std::byte* address) const;
  size_t getSize() const;

  explicit DmaBuffer(std::unique_ptr<DmaBufferImp> impl, DmaBufferManager* dmaBufferManager);
  ~DmaBuffer();

  DmaBuffer(DmaBuffer&&) noexcept;
  DmaBuffer& operator=(DmaBuffer&&) noexcept;

private:
  std::unique_ptr<DmaBufferImp> impl_;
  DmaBufferManager* dmaBufferManager_;
};
} // namespace rt