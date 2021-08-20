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
#include "runtime/Types.h"
#include <bitset>
#include <cstddef>
#include <functional>
#include <sstream>

namespace rt {
constexpr auto kNumPages = 256;
constexpr auto kPageSize = 1U << 20;
class PageManager;
class PageGroup {
public:
  PageGroup(PageManager* parent, uint16_t firstPage, uint16_t lastPage)
    : parent_(parent)
    , firstPage_(firstPage)
    , lastPage_(lastPage) {
  }
  std::byte* getPtr() const;
  PageGroup(PageGroup&&) noexcept;
  PageGroup& operator=(PageGroup&&) noexcept;
  ~PageGroup();

  PageManager* parent_;
  uint16_t firstPage_;
  uint16_t lastPage_;

private:
  PageGroup(const PageGroup&) = delete;
};

class PageManager {
public:
  using Allocator = std::function<std::byte*(size_t)>;
  using Deallocator = std::function<void(std::byte*)>;
  PageManager(const Allocator& allocator, const Deallocator& deallocator);
  ~PageManager();
  void free(const PageGroup* p);

  PageGroup alloc(size_t size);

  size_t getMaxSize() const {
    return maxSize_;
  }

  std::byte* getBaseAddress() const {
    return memory_;
  }

private:
  // it could be improved, but actually the number of pages is so small that it's not worth it. Everything should be
  // cached
  void updateMaxSize();
  static_assert(kNumPages % 2 == 0, "kNumPages should be pow2");
  Allocator allocator_;
  Deallocator deallocator_;
  std::bitset<kNumPages> busyPages_;
  size_t maxSize_ = kNumPages * kPageSize; // this value will be kept calculated always
  std::byte* memory_;
};
PageManager::~PageManager() {
  deallocator_(memory_);
}
PageManager::PageManager(const Allocator& allocator, const Deallocator& deallocator)
  : allocator_{allocator}
  , deallocator_{deallocator} {
  memory_ = allocator(kNumPages * kPageSize);
}

PageGroup PageManager::alloc(size_t size) {
  if (size == 0) {
    throw Exception("Error trying to allocate size 0");
  }
  if (size > maxSize_) {
    std::stringstream ss;
    ss << "Error trying to allocate size " << std::hex << size << " when max size is: " << maxSize_;
    throw Exception(ss.str());
  }
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
  return PageGroup(this, static_cast<uint16_t>(index - numPages), static_cast<uint16_t>(index - 1));
}

void PageManager::updateMaxSize() {
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

void PageManager::free(const PageGroup* p) {
  for (auto i = p->firstPage_; i <= p->lastPage_; ++i) {
    busyPages_.set(i, false);
  }
  updateMaxSize();
}

PageGroup::~PageGroup() {
  if (parent_) {
    parent_->free(this);
    parent_ = nullptr;
  }
}
std::byte* PageGroup::getPtr() const {
  return parent_->getBaseAddress() + (firstPage_ * kPageSize);
}
PageGroup::PageGroup(PageGroup&& other) noexcept
  : parent_(other.parent_)
  , firstPage_(other.firstPage_)
  , lastPage_(other.lastPage_) {
  other.parent_ = nullptr;
}

PageGroup& PageGroup::operator=(PageGroup&& other) noexcept {
  if (parent_) {
    parent_->free(this);
  }
  parent_ = other.parent_;
  firstPage_ = other.firstPage_;
  lastPage_ = other.lastPage_;
  other.parent_ = nullptr;
  return *this;
}

} // namespace rt