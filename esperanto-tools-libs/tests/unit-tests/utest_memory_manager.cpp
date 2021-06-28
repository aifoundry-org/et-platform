//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <device-layer/IDeviceLayerFake.h>
#include <gtest/gtest-death-test.h>
#include <hostUtils/logging/Logging.h>
#include <ios>

#define private public
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>

using namespace rt;

TEST(MemoryManagerDeathTest, ctor_invariants) {
  EXPECT_THROW({ auto mm = MemoryManager(3, 19872398); }, Exception);
  EXPECT_THROW({ auto mm = MemoryManager(0, 19872398); }, Exception);
  EXPECT_THROW({ auto mm = MemoryManager(0, 2046, 1023); }, Exception);
}

TEST(MemoryManager, compress_and_uncompress) {
  for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
    for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
      for (auto size : {1UL, 3UL, 12UL, 15UL}) {
        auto ptr = size * minAllocation + baseAddr;
        auto mm = MemoryManager(baseAddr, 1ul << 40, minAllocation);
        auto compressed = mm.compressPointer(reinterpret_cast<void*>(ptr));
        EXPECT_LE(compressed, ptr);
        auto uncompressed = mm.uncompressPointer(compressed);
        EXPECT_EQ(reinterpret_cast<decltype(ptr)>(uncompressed), ptr);
      }
    }
  }
}

TEST(MemoryManager, SW8240) {
  auto baseAddr = 0UL;
  auto size = (1 << 20) + 1UL;
  auto mm = MemoryManager(baseAddr, 1UL << 40);
  // shouldn't be possible to allocate a non multiple of alignment
  dev::IDeviceLayerFake deviceLayer;
  auto runtime = IRuntime::create(&deviceLayer);
  auto device = runtime->getDevices()[0];
  // now, it works because we are going through runtime
  auto ptr = runtime->mallocDevice(device, size);
  auto runtimeImp = static_cast<RuntimeImp*>(runtime.get());
  auto& memManager = runtimeImp->memoryManagers_.find(device)->second;
  auto allocated = memManager.allocated_.find(memManager.compressPointer(ptr));
  ASSERT_NE(allocated, end(memManager.allocated_));
  ASSERT_GE(allocated->second * kBlockSize, size);
}

using FreeChunk = MemoryManager::FreeChunk;
namespace rt {
bool operator==(const FreeChunk& lhs, const FreeChunk& rhs) {
  return lhs.size_ == rhs.size_ && lhs.startAddress_ == rhs.startAddress_;
}
std::ostream& operator<<(std::ostream& os, const FreeChunk& chunk) {
  os << "addr: " << chunk.startAddress_ << " size: " << chunk.size_;
  return os;
}
} // namespace rt

TEST(MemoryManager, addChunk) {
  auto totalRam = 1ul << 34;
  for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
    for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
      auto chunk1 = FreeChunk{1234, 10};
      auto chunk2 = FreeChunk{2000, 20};
      auto chunk3 = FreeChunk{5000, 30};
      {
        auto mm = MemoryManager(baseAddr, totalRam, minAllocation);
        mm.free_.clear();
        mm.addChunk(chunk1);
        EXPECT_EQ(mm.free_.front(), chunk1);
        mm.addChunk(chunk2);
        EXPECT_EQ(mm.free_.front(), chunk1);
        EXPECT_EQ(mm.free_.back(), chunk2);
        mm.addChunk(chunk3);
        EXPECT_EQ(mm.free_.front(), chunk1);
        EXPECT_EQ(mm.free_.back(), chunk3);
        EXPECT_EQ(mm.free_.size(), 3);

        // add a chunk which should merge with first chunk
        auto addSize = 100u;
        mm.addChunk({chunk1.startAddress_ + chunk1.size_, addSize});
        EXPECT_EQ(mm.free_.size(), 3);
        EXPECT_EQ(mm.free_.front().size_, chunk1.size_ + addSize);

        // add a chunk which should be put next to chunk2
        mm.addChunk({chunk2.startAddress_ + chunk2.size_ + 1, addSize});
        EXPECT_EQ(mm.free_.size(), 4);
        EXPECT_EQ(mm.free_[2].size_, addSize);
        EXPECT_EQ(mm.free_[2].startAddress_, chunk2.startAddress_ + chunk2.size_ + 1);

        auto addedAddress = 6000u;
        // add a chunk which should be put after last chunk
        mm.addChunk({chunk3.startAddress_ + addedAddress, addSize});
        EXPECT_EQ(mm.free_.size(), 5);
        EXPECT_EQ(mm.free_.back().size_, addSize);
        EXPECT_EQ(mm.free_.back().startAddress_, chunk3.startAddress_ + 6000);

        // add a chunk which should reduce the number of chunks in 1 (filling the gap between last and the one before)
        auto newChunkAddr = chunk3.startAddress_ + chunk3.size_;
        mm.addChunk({chunk3.startAddress_ + chunk3.size_, mm.free_.back().startAddress_ - newChunkAddr});
        EXPECT_EQ(mm.free_.size(), 4);
        auto b = mm.free_.back();
        EXPECT_EQ(b.startAddress_, chunk3.startAddress_);
        EXPECT_EQ(b.size_, addedAddress + addSize);
      }
    }
  }
}

TEST(MemoryManager, malloc_free_basic) {
  auto totalRam = 1UL << 34;
  for (auto alignment : {1U << 6, 1U << 8, 1U << 10, 1U << 11, 1U << 15}) {
    for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
      for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
        auto mm = MemoryManager(baseAddr, totalRam, minAllocation);
        std::vector<void*> ptrs;
        for (auto mallocSize : {1UL << 11, 1UL << 12, 1UL << 30}) {   
          auto ptr = mm.malloc(mallocSize, alignment);
          ASSERT_EQ(reinterpret_cast<uint64_t>(ptr) % alignment, 0UL);
          ptrs.emplace_back(ptr);
        }
        for (auto p : ptrs) {
          mm.free(p);
        }
        ASSERT_EQ(mm.free_.size(), 1);
      }
    }
  }
}

TEST(MemoryManager, malloc_free_holes) {
  auto totalRam = 1UL << 34;
  auto mm = MemoryManager(1 << 10, totalRam);

  EXPECT_THROW({ mm.malloc(totalRam + 1024, 1024); }, Exception);

  // alloc 100 buffers
  std::vector<void*> ptrs;
  for (int i = 0; i < 100; ++i) {
    ptrs.emplace_back(mm.malloc(1024, 1024));
    ASSERT_NE(ptrs.back(), nullptr);
  }
  ASSERT_EQ(mm.free_.size(), 1);

  // free odd pos of buffers
  for (auto i = 1U; i < 100; i += 2) {
    mm.free(ptrs[i]);
  }
  ASSERT_EQ(mm.free_.size(), 50);

  // free even buffers
  for (auto i = 0U; i < 100; i += 2) {
    mm.free(ptrs[i]);
  }
  ASSERT_EQ(mm.free_.size(), 1);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
