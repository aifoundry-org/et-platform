/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "utils.h"
#include "MemoryManager.h"
#include "runtime/IRuntime.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <ios>
using namespace rt;

MemoryManager::MemoryManager(uint64_t dramBaseAddr, size_t totalMemoryBytes, int minAllocationSize)
  : dramBaseAddr_(dramBaseAddr == 0 ? dramBaseAddr + minAllocationSize : dramBaseAddr)
  , totalMemoryBytes_(totalMemoryBytes)
  , minAlignmentLog2_(static_cast<int>(std::log2(minAllocationSize))) {

  if ((1 << minAlignmentLog2_) != minAllocationSize) {
    throw Exception("MinAllocationSize must be POT");
  }
  if (dramBaseAddr_ % (1 << minAlignmentLog2_)) {
    throw Exception("DramBaseAddr must be multiple of minAllocationSize");
  }
  if (totalMemoryBytes_ % (1 << minAlignmentLog2_)) {
    throw Exception("TotalMemoryBytes must be multiple of minAllocationSize");
  }
  free_.emplace_back(FreeChunk{0, static_cast<uint32_t>(totalMemoryBytes_ >> minAlignmentLog2_)});
}

void MemoryManager::addChunk(FreeChunk chunk) {
  if (free_.empty()) {
    free_.emplace_back(chunk);
    return;
  }

  auto tryToMerge = [](FreeChunk chunk, FreeChunk& target) {
    auto c0 = chunk;
    auto c1 = target;
    if (c1.startAddress_ < c0.startAddress_) {
      std::swap(c0, c1);
    }
    if (c0.startAddress_ + c0.size_ == c1.startAddress_) {
      target.startAddress_ = c0.startAddress_;
      target.size_ = c0.size_ + c1.size_;
      return true;
    }
    return false;
  };

  auto it = std::upper_bound(begin(free_), end(free_), chunk);
  if (it == begin(free_)) {
    // try to merge with next chunk, if not emplace at the beginning
    if (!tryToMerge(chunk, *it)) {
      free_.emplace(it, chunk);
    }
  } else if (it == end(free_)) {
    if (!tryToMerge(chunk, *(it - 1))) {
      free_.emplace(it, chunk);
    }
  } else {
    // try to merge with prev and next
    if (tryToMerge(chunk, *(it - 1))) {
      // on a merge with previous chunk, then try to merge with the next too
      if (tryToMerge(*it, *(it - 1))) {
        // remove the it since it was merged into the previous one
        free_.erase(it);
      }
    } else if (!tryToMerge(chunk, *it)) {
      free_.emplace(it, chunk);
    }
  }
}

void MemoryManager::free(void* ptr) {
  auto tmp = compressPointer(ptr);
  auto it = allocated_.find(tmp);
  if (it == allocated_.end()) {
    throw Exception("Ptr not allocated previously or double free");
  }
  addChunk(FreeChunk{it->first, it->second});
}

void* MemoryManager::malloc(size_t size, int alignment) {
  if ((1ul << __builtin_ctz(alignment)) != static_cast<size_t>(alignment) || alignment <= 0) {
    throw Exception("Alignment must be power of two and greater than 0");
  }
  if (size % alignment != 0) {
    throw Exception("Size must be multiple of alignment");
  }

  // size must be at least multiple of minAlignment
  size = std::max(1ul << minAlignmentLog2_, size);

  // calculate size in number of blocks
  auto countBlocks = size >> minAlignmentLog2_;

  // adjust alignment as requested and calculate extraBlocks
  auto alignmentBlocks = std::max(__builtin_ctz(alignment), minAlignmentLog2_) >> minAlignmentLog2_;

  // find a suitable chunk, if not return nullptr
  auto it = std::find_if(begin(free_), end(free_), [countBlocks, alignmentBlocks](const auto& elem) {
    return elem.size_ >= countBlocks + alignmentBlocks;
  });
  if (it == end(free_)) {
    throw Exception("Out of memory");
  }

  // fix the startAddress to match alignment
  auto addr = it->startAddress_;

  if (alignmentBlocks > 0 && addr % alignmentBlocks != 0) {
    auto padding = alignmentBlocks - (addr % alignmentBlocks);
    addr += padding;

    // update the existing chunk
    it->startAddress_ = addr + countBlocks;
    it->size_ -= (countBlocks + padding);

    // check if the chunk is empty
    if (it->size_ == 0) {
      free_.erase(it);
    }
  } else {
    it->startAddress_ += countBlocks;
    it->size_ -= countBlocks;
  }

  // bookkeep the allocation and return pointer
  allocated_.insert({addr, countBlocks});
  RT_DLOG(INFO) << "Malloc at device address: 0x" << std::hex << uncompressPointer(addr)
                << " compressed pointer (not address): 0x" << addr;
  return uncompressPointer(addr);
}
