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
#include "NewCore/MemoryManager.h"
#include <cstddef>
#include <mutex>
#include <runtime/IRuntime.h>
#include <set>
#include <unordered_map>
#include <vector>

namespace rt {
class KernelParametersCache {
public:
  explicit KernelParametersCache(IRuntime* runtime, int initialFreeListSize = 10, int bufferSize = kMinAllocationSize);

  // returns a buffer from the free list, if there are no free list then it will allocate a buffer and use that
  void* allocBuffer(DeviceId deviceId);

  // associates a previously allocated buffer through allocBuffer with the given kernelId.
  // The alloc + reserve is split in two steps since sometimes we don't know yet what will be the event to associate to
  // the buffer
  void reserveBuffer(EventId eventId, void* buffer);

  // release the buffer associated to given eventId and returns it to the freeBuffers list, should be done whenever
  // the kernel completes execution
  void releaseBuffer(EventId eventId);

  ~KernelParametersCache();

private:
  struct InUseBuffer {
    DeviceId device_;
    void* buffer_;
    bool operator<(const InUseBuffer& other) const {
      return buffer_ < other.buffer_;
    }
  };
  std::mutex mutex_;
  std::unordered_map<DeviceId, std::vector<void*>> freeBuffers_;
  std::unordered_map<EventId, InUseBuffer> reservedBuffers_;
  // these are the buffers which are currently allocated but yet not reserved.
  std::set<InUseBuffer> allocBuffers_;
  IRuntime* runtime_;
  int bufferSize_;
};
} // namespace rt