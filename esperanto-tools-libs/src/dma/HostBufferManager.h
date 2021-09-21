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
#include "runtime/IDmaBuffer.h"
#include "runtime/Types.h"
#include <bitset>
#include <cstddef>
#include <device-layer/IDeviceLayer.h>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace rt {
constexpr auto kNumPages = 256;
constexpr auto kPageSize = 1 << 20;
class HostBuffer;
class HostAllocation {
public:
  HostAllocation(HostBuffer* parent, size_t size, uint16_t firstPage, uint16_t lastPage)
    : parent_(parent)
    , size_(size)
    , firstPage_(firstPage)
    , lastPage_(lastPage) {
  }
  std::byte* getPtr() const;
  size_t getSize() const {
    return size_;
  }
  HostAllocation(HostAllocation&&) noexcept;
  HostAllocation& operator=(HostAllocation&&) noexcept;
  ~HostAllocation();

  HostBuffer* parent_;
  size_t size_;
  uint16_t firstPage_;
  uint16_t lastPage_;

private:
  HostAllocation(const HostAllocation&) = delete;
  HostAllocation& operator=(const HostAllocation&) = delete;
};

class HostBuffer {
public:
  explicit HostBuffer(std::unique_ptr<IDmaBuffer> dmaBuffer)
    : dmaBuffer_(std::move(dmaBuffer)) {
    maxSize_ = kPageSize * kNumPages;
  }
  void free(const HostAllocation* p);

  HostAllocation alloc(size_t size);

  size_t getMaxSize() const {
    std::lock_guard lock(mutex_);
    return maxSize_;
  }

  std::byte* getBaseAddress() const {
    return dmaBuffer_->getPtr();
  }
  ~HostBuffer();

private:
  // it could be improved, but actually the number of buffers is so small that it's not worth it.
  void updateMaxSize();

  static_assert(kNumPages % 2 == 0, "kNumPages should be pow2");
  std::bitset<kNumPages> busyPages_;
  std::unique_ptr<IDmaBuffer> dmaBuffer_;
  size_t maxSize_ = kNumPages * kPageSize;
  mutable std::mutex mutex_;
};

class HostBufferManager {
public:
  HostBufferManager(IRuntime* runtime, DeviceId device, uint32_t numBuffersInitially = 4);
  std::vector<HostAllocation> alloc(size_t size);
  ~HostBufferManager();

private:
  std::vector<std::unique_ptr<HostBuffer>> hostBuffers_;
  IRuntime* runtime_;
  DeviceId device_;
};

} // namespace rt