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

#include <cassert>
#include <cstddef>
#include <limits>
#include <map>
#include <vector>
namespace rt {
constexpr auto kBlockSize = 1024U;

class MemoryManager {
public:
  explicit MemoryManager(uint64_t dramBaseAddr, size_t totalMemoryBytes, uint32_t blockSize = kBlockSize);

  void* malloc(size_t size, uint32_t alignment);
  void free(void* ptr);

  uint32_t getBlockSize() const;

  void sanityCheck() const;

  size_t getFreeBytes() const;

  size_t getAllocatedBytes() const;

  size_t getNumAllocations() const;

  size_t getNumFreeChunks() const;

  void setDebugMode(bool enabled);

private:
  struct FreeChunk {
    uint32_t startAddress_;
    uint32_t size_;
    bool operator<(const FreeChunk& other) const {
      return startAddress_ < other.startAddress_;
    }
    bool empty() const {
      return size_ == 0U;
    }
    std::string str() const {
      return std::string("Address: ") + std::to_string(startAddress_) + "Size: " + std::to_string(size_);
    }
  };

  uint32_t compressPointer(void* ptr) const {
    auto tmp = reinterpret_cast<uint64_t>(ptr);
    tmp -= dramBaseAddr_;
    tmp >>= blockSizeLog2_;
    assert(tmp < std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(tmp);
  }
  void* uncompressPointer(uint32_t ptr) const {
    auto tmp = static_cast<uint64_t>(ptr);
    tmp <<= blockSizeLog2_;
    assert(tmp < totalMemoryBytes_);
    tmp += dramBaseAddr_;
    return reinterpret_cast<void*>(tmp);
  }

  void addChunk(FreeChunk chunk);

  std::map<uint32_t, uint32_t> allocated_;
  std::vector<FreeChunk> free_;
  uint64_t dramBaseAddr_;
  size_t totalMemoryBytes_;
  uint32_t blockSizeLog2_; // size of the minimum block in log2
  bool debugMode_ = false;
};
} // namespace rt