/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemoryManager.h"
#include "runtime/IRuntime.h"
#include <cstddef>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace rt {
class ExecutionContextCache {
public:
  struct Buffer {
    explicit Buffer(DeviceId device, IRuntime* runtime, size_t size);
    ~Buffer();
    std::byte* getExceptionContextPtr() const {
      return deviceBuffer_ + kBlockSize;
    }
    std::byte* getParametersPtr() const {
      return deviceBuffer_;
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(Buffer&&) = delete;
    std::vector<std::byte> hostBuffer_;
    std::byte* deviceBuffer_;
    DeviceId device_;
    IRuntime* runtime_;
  };

  explicit ExecutionContextCache(IRuntime* runtime, int initialFreeListSize = 10, int bufferSize = kBlockSize);

  // returns a buffer from the free list, if there are no free list then it will allocate a buffer and use that
  Buffer* allocBuffer(DeviceId deviceId);

  // associates a previously allocated buffer through allocBuffer with the given kernelId.
  // The alloc + reserve is split in two steps since sometimes we don't know yet what will be the event to associate to
  // the buffer
  void reserveBuffer(EventId eventId, Buffer* buffer);

  // release the buffer associated to given eventId and returns it to the freeBuffers list, should be done whenever
  // the kernel completes execution
  void releaseBuffer(EventId eventId);

  // returns an associated buffer for eventId or nullptr if there is no associated buffer
  Buffer* getReservedBuffer(EventId eventId) const;

  ~ExecutionContextCache();

private:
  mutable std::mutex mutex_;
  std::unordered_map<DeviceId, std::vector<Buffer*>> freeBuffers_;
  std::unordered_map<EventId, Buffer*> reservedBuffers_;
  // these are the buffers which are currently allocated but yet not reserved.
  std::set<Buffer*> allocBuffers_;

  // these are all the allocated buffers; (freeBuffers_ + reservedBuffers_ + allocBuffers_)
  std::vector<std::unique_ptr<Buffer>> buffers_;
  IRuntime* runtime_;
  int bufferSize_;
};
} // namespace rt
