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
#include <unordered_map>
#include <vector>
namespace rt {
constexpr auto kMinAllocationSize = 1024;

class MemoryManager {
public:
  explicit MemoryManager(uint64_t dramBaseAddr, size_t totalMemoryBytes, int minAllocationSize = kMinAllocationSize);

  void* malloc(size_t size, int alignment);
  void free(void* ptr);

private:
  struct FreeChunk {
    uint32_t startAddress_;
    uint32_t size_;
    bool operator<(const FreeChunk& other) const {
      return startAddress_ < other.startAddress_;
    }
  }; 

  uint32_t compressPointer(void* ptr) const {
    auto tmp = reinterpret_cast<uint64_t>(ptr);
    tmp -= dramBaseAddr_;
    tmp >>= minAlignmentLog2_;
    assert(tmp < std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(tmp);
  }
  void* uncompressPointer(uint32_t ptr) const {
    auto tmp = static_cast<uint64_t>(ptr);
    tmp <<= minAlignmentLog2_;
    assert(tmp < totalMemoryBytes_);
    tmp += dramBaseAddr_;
    return reinterpret_cast<void*>(tmp);
  }

  void addChunk(FreeChunk chunk);

  std::unordered_map<uint32_t, uint32_t> allocated_;
  std::vector<FreeChunk> free_;
  uint64_t dramBaseAddr_;
  size_t totalMemoryBytes_;
  int minAlignmentLog2_; // size of the minimum block in log2
};
} // namespace rt