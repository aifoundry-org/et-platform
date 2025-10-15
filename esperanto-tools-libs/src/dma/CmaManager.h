/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemoryManager.h"
#include "runtime/Types.h"
#include <condition_variable>
#include <cstddef>
#include <hostUtils/actionList/Runner.h>
#include <mutex>
namespace rt {
class IRuntime;
class CmaManager {
public:
  explicit CmaManager(std::unique_ptr<IDmaBuffer> dmaBuffer, size_t maxBytesPerCommand);

  // returns total size
  size_t getTotalSize() const;

  // returns max contiguous bytes (max allocation)
  size_t getFreeBytes() const;

  // will return nullptr if there is not enough mem to alloc.
  std::byte* alloc(size_t size);

  void free(std::byte* buffer);

  // add an asynchronous memcpy operation to be executed
  void addMemcpyAction(std::unique_ptr<actionList::IAction> action);

private:
  actionList::Runner memcpyActionManager_;
  std::unique_ptr<IDmaBuffer> dmaBuffer_;
  MemoryManager memoryManager_;
  std::condition_variable cv_;
  mutable std::mutex mutex_;
  const size_t maxBytesPerCommand_;
};
} // namespace rt