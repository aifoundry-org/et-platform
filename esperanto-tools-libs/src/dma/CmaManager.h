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
#include "MemoryManager.h"
#include "runtime/Types.h"
#include <condition_variable>
#include <cstddef>
#include <mutex>
namespace rt {
class IRuntime;
class CmaManager {
public:
  explicit CmaManager(std::unique_ptr<IDmaBuffer> dmaBuffer, size_t maxBytesPerCommand);

  // returns total size
  size_t getTotalSize() const;

  // returns max contiguous bytes (max allocation)
  size_t getFreeBytes(bool isPrioritary) const;

  // will return nullptr if there is not enough mem to alloc. When isPrioritary = false, it will allow to allocate a
  // maximum of totalFreeBytes - maxBytesPerCommand. Prioritary commands however, can allocate all available memory
  std::byte* alloc(size_t size, bool isPrioritary);

  void free(std::byte* buffer);

  // will block the caller until someone frees this amount of cma memory
  void waitUntilFree(size_t size);

private:
  std::unique_ptr<IDmaBuffer> dmaBuffer_;
  MemoryManager memoryManager_;
  std::condition_variable cv_;
  mutable std::mutex mutex_;
  const size_t maxBytesPerCommand_;
};
} // namespace rt