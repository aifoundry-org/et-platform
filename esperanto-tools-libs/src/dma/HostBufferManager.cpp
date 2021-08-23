/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "HostBufferManager.h"
#include "runtime/Types.h"
#include <sstream>

using namespace rt;

HostBuffer::~HostBuffer() {
  if (memory_) {
    deallocator_(memory_);
  }
}
HostBuffer::HostBuffer(const Allocator& allocator, const Deallocator& deallocator)
  : allocator_{allocator}
  , deallocator_{deallocator} {
  memory_ = allocator_(kNumPages * kPageSize);
}

HostAllocation HostBuffer::alloc(size_t size) {
  if (size == 0) {
    throw Exception("Error trying to allocate size 0");
  }
  if (size > maxSize_) {
    std::stringstream ss;
    ss << "Error trying to allocate size " << std::hex << size << " when max size is: " << maxSize_;
    throw Exception(ss.str());
  }
  std::lock_guard lock(mutex_);
  auto numPages = (size + kPageSize - 1) / kPageSize;
  auto index = 0U;

  // find a good spot to allocate the pagegroup. Simple greddy algorithm
  for (auto freePages = 0U; freePages < numPages; ++index) {
    if (!busyPages_.test(index)) {
      freePages++;
    } else {
      freePages = 0;
    }
  }
  // now the range corresponding to the alloc is [index-numPages, index). Mark as busy these pages
  for (auto i = index - numPages; i < index; ++i) {
    busyPages_.set(i);
  }

  // recalculate maxSize
  updateMaxSize();

  // now return the PageGroup
  return HostAllocation(this, static_cast<uint16_t>(index - numPages), static_cast<uint16_t>(index - 1));
}

void HostBuffer::updateMaxSize() {
  maxSize_ = 0;
  auto currentSize = 0UL;
  for (auto i = 0U; i < kNumPages; ++i) {
    if (!busyPages_.test(i)) {
      currentSize++;
    } else {
      maxSize_ = std::max(maxSize_, currentSize * kPageSize);
      currentSize = 0UL;
    }
  }
  // after the loop, last check
  maxSize_ = std::max(maxSize_, currentSize * kPageSize);
}

void HostBuffer::free(const HostAllocation* p) {
  std::lock_guard lock(mutex_);
  for (auto i = p->firstPage_; i <= p->lastPage_; ++i) {
    busyPages_.set(i, false);
  }
  updateMaxSize();
}

HostAllocation::~HostAllocation() {
  if (parent_) {
    parent_->free(this);
    parent_ = nullptr;
  }
}
std::byte* HostAllocation::getPtr() const {
  return parent_->getBaseAddress() + (firstPage_ * kPageSize);
}

size_t HostAllocation::getSize() const {
  return static_cast<size_t>(lastPage_ - firstPage_ + 1) * kPageSize;
}

HostAllocation::HostAllocation(HostAllocation&& other) noexcept
  : parent_(other.parent_)
  , firstPage_(other.firstPage_)
  , lastPage_(other.lastPage_) {
  other.parent_ = nullptr;
}

HostAllocation& HostAllocation::operator=(HostAllocation&& other) noexcept {
  if (parent_) {
    parent_->free(this);
  }
  parent_ = other.parent_;
  firstPage_ = other.firstPage_;
  lastPage_ = other.lastPage_;
  other.parent_ = nullptr;
  return *this;
}

HostBufferManager::HostBufferManager(const Allocator& allocator, const Deallocator& deallocator,
                                     uint32_t numBuffersInitially)
  : allocator_(allocator)
  , deallocator_(deallocator) {
  for (auto i = 0U; i < numBuffersInitially; ++i) {
    hostBuffers_.emplace_back(std::make_unique<HostBuffer>(allocator_, deallocator_));
  }
}

std::vector<HostAllocation> HostBufferManager::alloc(size_t size) {
  size_t remainingSize = size;
  std::lock_guard lock(mutex_);
  std::vector<HostAllocation> result;

  auto doAllocate = [&result](auto& hostBuffer, auto& remSize) {
    auto allocSize = std::min(hostBuffer->getMaxSize(), remSize);
    result.emplace_back(hostBuffer->alloc(allocSize));
    remSize -= allocSize;
  };

  // first fill with existing buffers
  for (auto it = begin(hostBuffers_); it != end(hostBuffers_) && remainingSize > 0; ++it) {
    auto& hb = *it;
    if (hb->getMaxSize() > 0) {
      // since nobody can alloc at this time (because of the mutex), there is no risk to DECREASE the getMaxSize. It
      // could increase tho, if someone did free a HostAllocation between this call and the allocation call itself. But
      // that does not break this operation.
      doAllocate(hb, remainingSize);
    }
  }

  // if there is still no enough memory, we need to allocate more buffers
  for (auto numBuffers = (remainingSize + kPageSize - 1) / kPageSize; numBuffers > 0; --numBuffers) {
    auto hb = std::make_unique<HostBuffer>(allocator_, deallocator_);
    doAllocate(hb, remainingSize);
    hostBuffers_.emplace_back(std::move(hb));
  }
  return result;
}