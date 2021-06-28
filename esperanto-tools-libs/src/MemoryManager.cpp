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

MemoryManager::MemoryManager(uint64_t dramBaseAddr, size_t totalMemoryBytes, uint32_t blockSize)
  : dramBaseAddr_(dramBaseAddr == 0 ? dramBaseAddr + blockSize : dramBaseAddr)
  , totalMemoryBytes_(totalMemoryBytes)
  , blockSizeLog2_(static_cast<uint32_t>(std::log2(blockSize))) {

  if ((1U << blockSizeLog2_) != blockSize) {
    throw Exception("BlockSize must be POT");
  }
  if (dramBaseAddr_ % (1U << blockSizeLog2_)) {
    throw Exception("DramBaseAddr must be multiple of BlockSize");
  }
  if (totalMemoryBytes_ % (1U << blockSizeLog2_)) {
    throw Exception("TotalMemoryBytes must be multiple of BlockSize");
  }
  free_.emplace_back(FreeChunk{0, static_cast<uint32_t>(totalMemoryBytes_ >> blockSizeLog2_)});
}

void MemoryManager::addChunk(FreeChunk chunk) {
  if (free_.empty()) {
    free_.emplace_back(chunk);
    return;
  }

  auto tryToMerge = [](FreeChunk c, FreeChunk& target) {
    auto c0 = c;
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

void* MemoryManager::malloc(size_t size, uint32_t alignment) {
  // calculate size in number of blocks
  auto blockSize = getBlockSize();

  // if alignment is lower than blockSize, then we force the alignment to blockSize
  alignment = std::max(alignment, blockSize);

  auto countBlocks = static_cast<uint32_t>((size + blockSize - 1) / blockSize);

  // extra potential blocks for alignment
  auto extraBlocks = alignment / blockSize - 1;

  auto totalBlocks = countBlocks + extraBlocks;

  // find a suitable chunk, if not return nullptr
  auto it =
    std::find_if(begin(free_), end(free_), [totalBlocks](const auto& elem) { return elem.size_ >= totalBlocks; });
  if (it == end(free_)) {
    throw Exception("Out of memory");
  }

  // calculate the alignment in blocks
  auto addr = it->startAddress_;  
  auto tmp = reinterpret_cast<uint64_t>(uncompressPointer(addr));
  auto extraBytes = (tmp % alignment == 0) ? 0 : static_cast<uint32_t>(alignment - tmp % alignment);

  // fullfill the requested alignment
  auto missAlignment = extraBytes / blockSize;

  // take countBlocks from freeChunk
  it->startAddress_ += countBlocks + missAlignment;
  it->size_ -= countBlocks + missAlignment;

  // if we needed extra blocks for alignment, add a freeChunk with those extra blocks (which are actually not used)
  if (missAlignment > 0) {
    addChunk(FreeChunk{addr, missAlignment});
  }
  // bookkeep the allocation and return pointer
  addr += missAlignment;
  allocated_.insert({addr, countBlocks});

  RT_DLOG(INFO) << "Malloc at device address: " << std::hex << uncompressPointer(addr)
                << " compressed pointer (not address): " << addr;
  return uncompressPointer(addr);
}

uint32_t MemoryManager::getBlockSize() const {
  return 1U << blockSizeLog2_;
}
