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
#include <bitset>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace rt {
constexpr auto kNumPages = 256;
constexpr auto kPageSize = 1U << 20;
class HostBuffer;
class HostAllocation {
public:
  HostAllocation(HostBuffer* parent, uint16_t firstPage, uint16_t lastPage)
    : parent_(parent)
    , firstPage_(firstPage)
    , lastPage_(lastPage) {
  }
  std::byte* getPtr() const;
  size_t getSize() const;
  HostAllocation(HostAllocation&&) noexcept;
  HostAllocation& operator=(HostAllocation&&) noexcept;
  ~HostAllocation();

  HostBuffer* parent_;
  uint16_t firstPage_;
  uint16_t lastPage_;

private:
  HostAllocation(const HostAllocation&) = delete;
};

class HostBuffer {
public:
  using Allocator = std::function<std::byte*(size_t)>;
  using Deallocator = std::function<void(std::byte*)>;
  HostBuffer(const Allocator& allocator, const Deallocator& deallocator);
  ~HostBuffer();
  void free(const HostAllocation* p);

  HostAllocation alloc(size_t size);

  size_t getMaxSize() const {
    std::lock_guard lock(mutex_);
    return maxSize_;
  }

  std::byte* getBaseAddress() const {
    return memory_;
  }

private:
  HostBuffer(const HostBuffer&) = delete;
  // it could be improved, but actually the number of buffers is so small that it's not worth it.
  void updateMaxSize();
  static_assert(kNumPages % 2 == 0, "kNumPages should be pow2");
  std::bitset<kNumPages> busyPages_;
  size_t maxSize_ = kNumPages * kPageSize; // this value will be kept calculated always
  mutable std::mutex mutex_;
  const Allocator& allocator_;
  const Deallocator& deallocator_;
  std::byte* memory_;
};

class HostBufferManager {
public:
  using Allocator = HostBuffer::Allocator;
  using Deallocator = HostBuffer::Deallocator;

  HostBufferManager(const Allocator& allocator, const Deallocator& deallocator, uint32_t numBuffersInitially = 4);
  std::vector<HostAllocation> alloc(size_t size);

private:
  Allocator allocator_;
  Deallocator deallocator_;
  std::vector<std::unique_ptr<HostBuffer>> hostBuffers_;
  std::mutex mutex_;
};

} // namespace rt