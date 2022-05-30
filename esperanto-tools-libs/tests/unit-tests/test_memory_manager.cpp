//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include <algorithm>
#include <gmock/gmock-actions.h>
#include <gtest/gtest-death-test.h>
#include <hostUtils/logging/Logging.h>
#include <ios>
#include <random>
#include <string_view>
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>

using namespace rt;

TEST(MemoryManagerDeathTest, ctor_invariants) {
  EXPECT_THROW({ auto mm = MemoryManager(0, 2046, 1023); }, Exception);
}

TEST(MemoryManager, compress_and_uncompress) {
  for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
    for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
      for (auto size : {1UL, 3UL, 12UL, 15UL}) {
        auto ptr = size * minAllocation + baseAddr;
        auto mm = MemoryManager(baseAddr, 1UL << 40, minAllocation);
        auto compressed = mm.compressPointer(reinterpret_cast<std::byte*>(ptr));
        EXPECT_LE(compressed, ptr);
        auto uncompressed = mm.uncompressPointer(compressed);
        EXPECT_EQ(reinterpret_cast<decltype(ptr)>(uncompressed), ptr);
      }
    }
  }
}

TEST(MemoryManager, SW8240) {
  auto size = (1 << 20) + 1UL;
  // shouldn't be possible to allocate a non multiple of alignment
  dev::DeviceLayerFake deviceLayer;
  auto runtime = IRuntime::create(&deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto device = runtime->getDevices()[0];
  // now, it works because we are going through runtime
  auto ptr = runtime->mallocDevice(device, size);
  auto runtimeImp = static_cast<RuntimeImp*>(runtime.get());
  auto& memManager = runtimeImp->memoryManagers_.find(device)->second;
  ASSERT_NE(ptr, nullptr);
  auto allocated = memManager.allocated_.find(memManager.compressPointer(ptr));
  ASSERT_NE(allocated, end(memManager.allocated_));
  ASSERT_GE(allocated->second * kBlockSize, size);
}

TEST(MemoryManager, SW8240_2) {
  dev::DeviceLayerFake deviceLayer;
  auto runtime = IRuntime::create(&deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto device = runtime->getDevices()[0];
  auto rimp = static_cast<RuntimeImp*>(runtime.get());
  rimp->setMemoryManagerDebugMode(device, true);
  static constexpr auto sizes =
    std::array{1024,   1024,   1024,   1024,   1024,   1024,   1024,   1024,   1024,   1024,   377600, 27754496, 540672,
               925696, 540672, 472,    925696, 472,    540672, 925696, 472,    540672, 925696, 472,    540672,   925696,
               472,    540672, 925696, 472,    540672, 925696, 472,    540672, 925696, 472,    540672, 925696,   472,
               540672, 925696, 472,    540672, 925696, 472,    540672, 925696, 472,    540672, 925696, 472,      540672,
               925696, 472,    540672, 925696, 472,    540672, 925696, 472,    540672, 540672, 925696, 925696,   472,
               472,    540672, 925696, 472,    540672, 925696, 472,    540672, 925696, 472,    540672, 925696,   472,
               540672, 925696, 472,    540672, 925696, 472,    540672, 925696, 472};

  std::vector<std::thread> threads;
  for (auto i = 0; i < 50; ++i) {
    threads.emplace_back(std::thread([rimp, device] {
      for (auto s : sizes) {
        rimp->mallocDevice(device, static_cast<uint32_t>(s));
      }
      RT_LOG(INFO) << "End mallocs";
    }));
  }
  for (auto& t : threads) {
    t.join();
  }
}
TEST(MemoryManager, SW8240_3) {
  dev::DeviceLayerFake deviceLayer;
  auto runtime = IRuntime::create(&deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto device = runtime->getDevices()[0];
  auto rimp = static_cast<RuntimeImp*>(runtime.get());
  rimp->setMemoryManagerDebugMode(device, true);
  for (int i = 0; i < 1000; ++i) {
    auto ptr = rimp->mallocDevice(device, 0xe2000);
    RT_LOG(INFO) << "Malloc addr: " << std::hex << ptr;
  }
}

TEST(MemoryManager, SW8240_4) {
  dev::DeviceLayerFake deviceLayer;
  auto runtime = IRuntime::create(&deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto device = runtime->getDevices()[0];
  auto rimp = static_cast<RuntimeImp*>(runtime.get());
  rimp->setMemoryManagerDebugMode(device, true);

  std::default_random_engine e1(12389);
  std::uniform_int_distribution<uint32_t> uniform_dist(1U, 1U << 29);
  std::vector<std::byte*> ptrs;
  auto&& mm = rimp->memoryManagers_.find(device)->second;

  for (int i = 0; i < 10000; ++i) {
    auto size = uniform_dist(e1);
    RT_LOG(INFO) << std::dec << "Iter: " << i << std::hex << " Size: " << size;
    bool done = false;
    while (!done) {
      try {
        ptrs.emplace_back(rimp->mallocDevice(device, size, kCacheLineSize));
        done = true;
      } catch (const rt::Exception& e) {
        if (std::string_view(e.what()).find("Out of memory")) {
          RT_LOG(INFO) << std::hex << "Requesting " << size
                       << " but free space is not enough (perhaps there is some fragmentation) " << mm.getFreeBytes();
          std::shuffle(begin(ptrs), end(ptrs), e1);
          auto freeBefore = mm.getFreeBytes();
          rimp->freeDevice(device, ptrs.back());
          auto freeAfter = mm.getFreeBytes();
          ASSERT_GT(freeAfter, freeBefore);
          ptrs.pop_back();
        } else {
          RT_LOG(FATAL) << std::dec << "Exception iteration: " << i;
        }
      }
    }
  }
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
  auto totalRam = 1UL << 34;
  for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
    for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
      auto chunk1 = FreeChunk{1234, 10};
      auto chunk2 = FreeChunk{2000, 20};
      auto chunk3 = FreeChunk{5000, 30};
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

TEST(MemoryManager, malloc_free_basic) {
  auto totalRam = 1UL << 34;
  for (auto alignment : {1U << 6, 1U << 8, 1U << 10, 1U << 11, 1U << 15}) {
    for (auto baseAddr : {0UL, 1UL << 13, 1UL << 20}) {
      for (auto minAllocation : {1U << 10, 1U << 11, 1U << 13}) {
        auto mm = MemoryManager(baseAddr, totalRam, minAllocation);
        mm.setDebugMode(true);
        std::vector<std::byte*> ptrs;
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

TEST(MemoryManager, contiguous_bytes) {
  auto totalRam = 1UL << 34;
  auto mm = MemoryManager(1UL << 13, totalRam, kBlockSize);
  ASSERT_EQ(mm.getFreeBytes(), mm.getFreeContiguousBytes());
  ASSERT_EQ(mm.getFreeBytes(), totalRam);
  auto alloc1 = mm.malloc(totalRam / 2, kBlockSize);
  ASSERT_EQ(mm.getFreeBytes(), mm.getFreeContiguousBytes());
  ASSERT_EQ(mm.getFreeBytes(), totalRam / 2);
  auto alloc2 = mm.malloc(totalRam / 4, kBlockSize);
  ASSERT_EQ(mm.getFreeBytes(), mm.getFreeContiguousBytes());
  ASSERT_EQ(mm.getFreeBytes(), totalRam / 4);
  mm.free(alloc1);
  ASSERT_EQ(mm.getFreeBytes(), 3 * totalRam / 4);
  ASSERT_EQ(mm.getFreeContiguousBytes(), totalRam / 2);
  mm.free(alloc2);
  ASSERT_EQ(mm.getFreeBytes(), mm.getFreeContiguousBytes());
  ASSERT_EQ(mm.getFreeBytes(), totalRam);
}

TEST(MemoryManager, malloc_free_holes) {
  auto totalRam = 1UL << 34;
  auto mm = MemoryManager(1 << 12, totalRam);
  mm.setDebugMode(true);

  EXPECT_THROW({ mm.malloc(totalRam + 1024, 1024); }, Exception);

  // alloc 100 buffers
  std::vector<std::byte*> ptrs;
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

TEST(MemoryManager, check_operation) {
  auto totalRam = 1UL << 34;
  auto startAddress = 1UL << 12;
  for (auto blockSize : {128U, 1024U, 2048U, 4096U}) {
    RT_DLOG(INFO) << "BlockSize: " << blockSize;
      for (auto allocSize : {1U << 12, 1U << 20, 1U << 23, blockSize}) {
        RT_DLOG(INFO) << "AllocSize: " << allocSize;
        std::vector<std::byte*> allocations;
        auto mm = MemoryManager(startAddress, totalRam, blockSize);
        ASSERT_THROW(mm.checkOperation(reinterpret_cast<std::byte*>(startAddress), 1), rt::Exception);
        for (auto i = 0U; i < 10; ++i) {
          RT_DLOG(INFO) << "Iter: " << i;
          allocations.emplace_back(mm.malloc(allocSize, allocSize));
          ASSERT_NO_THROW(mm.checkOperation(allocations.back(), allocSize));
          ASSERT_THROW(mm.checkOperation(allocations.back(), allocSize + 1), rt::Exception);
          ASSERT_NO_THROW(mm.checkOperation(allocations.back() + allocSize / 2, allocSize / 2));
          ASSERT_NO_THROW(mm.checkOperation(*allocations.begin(), allocSize * (i + 1)));
          ASSERT_THROW({ mm.checkOperation(*allocations.begin(), allocSize * (i + 1) + 1); }, rt::Exception);
        }
        mm.free(allocations[4]);
        ASSERT_THROW(mm.checkOperation(allocations[4], 1), rt::Exception);
        ASSERT_THROW(mm.checkOperation(allocations[0], allocSize * 4 + 1), rt::Exception);
        ASSERT_NO_THROW(mm.checkOperation(allocations[0], allocSize * 4));
        ASSERT_NO_THROW(mm.checkOperation(allocations[5], allocSize * 5));
        ASSERT_THROW(mm.checkOperation(allocations[5], allocSize * 5 + 1), rt::Exception);
        ASSERT_THROW(mm.checkOperation(allocations[5] + 1, allocSize * 5), rt::Exception);
      }
  }
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
