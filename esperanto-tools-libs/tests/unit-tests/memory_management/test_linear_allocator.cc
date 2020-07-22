//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagement/LinearAllocator.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace et_runtime::device::memory_management;

class TestLinearAllocator : public ::testing::Test {

public:
  std::unique_ptr<LinearAllocator> allocator;

  std::list<BufferInfo<FreeRegion>> &free_list() {
    return allocator->free_list_;
  }
  std::list<std::shared_ptr<AbstractBufferInfo>> &allocated_list() {
    return allocator->allocated_list_;
  }

  auto findAllocatedBuffer(et_runtime::BufferID tid) {
    return allocator->findAllocatedBuffer(tid);
  }
};

TEST(List, prev) {
  std::list<int> a = {1};
  auto bit = a.begin();
  auto prev_bit = std::prev(bit);
  ASSERT_EQ(prev_bit, a.end());
}

// Expect that this malloc will fail because we do not have space for the
// tensor meta-data
TEST_F(TestLinearAllocator, malloc_fail_md_size) {
  allocator.reset(new LinearAllocator(100, 100));
  auto res = allocator->malloc(BufferType::Code, 100, 0);

  ASSERT_FALSE((bool)res);
}

// Single memory allocations
TEST_F(TestLinearAllocator, single_malloc_success) {
  auto base_start = 100;
  auto type = BufferType::Code;
  auto buffer_size = 10;
  auto total_size = buffer_size + allocator->mdSize(type);

  allocator.reset(new LinearAllocator(base_start, total_size));
  auto res = allocator->malloc(type, buffer_size, 0);

  ASSERT_TRUE((bool)res);

  // We expect to have allocated the first tensor
  auto tid = res.get();
  EXPECT_NE(tid, 0);

  // The list should be empty now
  ASSERT_TRUE(free_list().empty());

  auto free_res = allocator->free(tid);
  ASSERT_TRUE((bool)res);
  ASSERT_EQ(free_list().size(), 1);
  auto free_entry = free_list().front();

  ASSERT_EQ(free_entry.base(), base_start);
  ASSERT_EQ(free_entry.size(), total_size);
  ASSERT_TRUE(allocated_list().empty());
}

// Single memory allocations where we align the pointer to
// different addresses
TEST_F(TestLinearAllocator, single_malloc_aligned_success) {
  auto base_start = 100;
  auto max_alignment = 1ULL << 14; // 16KB
  auto type = BufferType::Code;
  auto buffer_size = 10;
  auto total_size = buffer_size + allocator->mdSize(type) + max_alignment * 2;

  allocator.reset(new LinearAllocator(base_start, total_size));

  for (int i = 0; i < 14; i++) {
    for (int j = 0; j <= i; j++) {
      auto alignment = 1ULL << i;
      auto size = buffer_size + (1ULL << j);
      auto res = allocator->malloc(type, size, alignment);

      ASSERT_TRUE((bool)res);

      // allocator->printState();

      // We expect to have allocated the first tensor
      auto tid = res.get();

      auto tensor = allocated_list().back();
      ASSERT_EQ(tensor->alignedStart() % alignment, 0);

      auto free_res = allocator->free(tid);
      ASSERT_TRUE((bool)res);
      ASSERT_EQ(free_list().size(), 1);
      auto free_entry = free_list().front();

      ASSERT_EQ(free_entry.base(), base_start);
      ASSERT_EQ(free_entry.size(), total_size);
      ASSERT_TRUE(allocated_list().empty());
    }
  }
}

// Multiple allocations
TEST_F(TestLinearAllocator, multiple_malloc_success) {

  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  BufferSizeTy tensor_size = 20;
  BufferOffsetTy dram_base = 100;
  auto dram_size = 3 * (tensor_size + md_size);

  allocator.reset(new LinearAllocator(dram_base, dram_size));

  ASSERT_EQ(free_list().front().size(), dram_size);
  /// TODO check the double linked list meta-data

  // First allocation
  {
    auto res = allocator->malloc(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    ASSERT_EQ(free_list().front().base(), dram_base + (tensor_size + md_size));
    ASSERT_EQ(free_list().front().size(), dram_size - (tensor_size + md_size));

    auto tensor = allocated_list().back();
    ASSERT_EQ(tensor->alignedStart(), dram_base + md_size);
  }

  // Second allocation
  {
    auto res = allocator->malloc(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    ASSERT_EQ(free_list().front().base(),
              dram_base + 2 * (tensor_size + md_size));
    ASSERT_EQ(free_list().front().size(),
              dram_size - 2 * (tensor_size + md_size));

    auto tensor = allocated_list().back();
    ASSERT_EQ(tensor->alignedStart(), dram_base + 2 * md_size + tensor_size);
  }

  // Third allocation
  {
    auto res = allocator->malloc(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    ASSERT_TRUE(free_list().empty());

    auto tensor = allocated_list().back();
    ASSERT_EQ(tensor->alignedStart(),
              dram_base + md_size + 2 * (md_size + tensor_size));
  }

  allocator->printStateJSON();
}

// parametrized test class, receives as parameters a tuple
class TestLinearAllocatorEmplaceFailure
    : public TestLinearAllocator,
      public testing::WithParamInterface<std::tuple<int, int, int, int>> {};

// Memory Emplacements expected errors
TEST_P(TestLinearAllocatorEmplaceFailure, single_emplace_failures) {
  auto [region_start, region_size, emplace_start, emplace_size] = GetParam();
  allocator.reset(new LinearAllocator(region_start, region_size));
  auto type = BufferType::Code;
  auto res = allocator->emplace(type, emplace_start, emplace_size);

  ASSERT_FALSE((bool)res);
}

INSTANTIATE_TEST_CASE_P(
    EmplaceFail, TestLinearAllocatorEmplaceFailure,
    testing::Values(
        // region start, region_size, emplace_start emplace size
        // Start offset outside region
        std::tuple<int, int, int, int>{100, 100, 0, 1},
        std::tuple<int, int, int, int>{100, 100, 100, 1},
        // Meta-data size increases buffer size and cannot fit
        std::tuple<int, int, int, int>{100, 100, 100, 100},
        std::tuple<int, int, int, int>{100, 100, 120, 79}));

// parametrized test class, receives as parameters a tuple
class TestLinearAllocatorEmplaceSuccess
    : public TestLinearAllocator,
      public testing::WithParamInterface<std::tuple<int, int, int, int, int>> {
};

// Memory Emplacements expected errors
TEST_P(TestLinearAllocatorEmplaceSuccess, single_emplace_success) {
  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  auto [region_start, left_free, emplace_size, right_free, free_list_size] =
      GetParam();
  auto emplace_start = region_start + md_size + left_free;
  auto region_size = emplace_start + emplace_size + right_free - region_start;
  allocator.reset(new LinearAllocator(region_start, region_size));

  auto res = allocator->emplace(type, emplace_start, emplace_size);

  allocator->printState();

  ASSERT_TRUE((bool)res);

  ASSERT_EQ(free_list().size(), free_list_size);
}

INSTANTIATE_TEST_CASE_P(
    EmplaceSuccess, TestLinearAllocatorEmplaceSuccess,
    testing::Values(
        // region start, left_free, emplaze_size, right_Free, free_list_size
        std::tuple<int, int, int, int, int>{100, 10, 10, 10, 2},
        std::tuple<int, int, int, int, int>{100, 0, 10, 10, 1},
        std::tuple<int, int, int, int, int>{100, 10, 10, 0, 1},
        std::tuple<int, int, int, int, int>{100, 0, 10, 0, 0}));

// parametried test class, receives as parameters a vector of tuples
class TestLinearAllocatorMallocFree
    : public TestLinearAllocator,
      public testing::WithParamInterface<std::vector<std::tuple<int, int>>> {};

// Multiple allocations and frees, change the deallocation order
// The vector of tuples has pas parameters <tensor_pos, free_list_size>
// We are modifying the order by which we are de-allocating tesnors and
// we check the number of free-list regions that are being created.
// Based on the order of deallocation adjacent regions that get freed
// shold be merged into a larger region. At the end we should have the original
// total free space
// In this test no 'gap" is allowed betwen the tensors
TEST_P(TestLinearAllocatorMallocFree, malloc_free_random) {

  std::vector<std::tuple<int, int>> tensor_free_order = GetParam();
  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  std::vector<BufferSizeTy> tensor_size = {20, 30, 40};
  std::vector<et_runtime::BufferID> tensors = {};
  BufferOffsetTy dram_base = 0;
  auto dram_size = 0;
  for (auto &i : tensor_size) {
    dram_size += i + md_size;
  }

  allocator.reset(new LinearAllocator(dram_base, dram_size));

  for (auto size : tensor_size) {
    auto res = allocator->malloc(type, size, 0);
    ASSERT_TRUE((bool)res);
    tensors.push_back(res.get());
  }

  // allocator->printState();

  ASSERT_TRUE(free_list().empty());

  auto total_free_size = allocator->freeMemory();

  // allocator->printState();

  for (auto i : tensor_free_order) {
    auto &[tensor, free_list_size] = i;
    auto tid = tensors[tensor];
    auto tit = findAllocatedBuffer(tid);
    total_free_size += (*tit)->size();
    auto free_res = allocator->free(tid);
    // allocator->printState();
    ASSERT_EQ(free_res, etrtSuccess);
    ASSERT_EQ(free_list().size(), free_list_size);
    ASSERT_EQ(total_free_size, allocator->freeMemory());
  }
  ASSERT_EQ(free_list().front().size(), dram_size);
}

INSTANTIATE_TEST_CASE_P(
    MallocFree, TestLinearAllocatorMallocFree,
    testing::Values(
        // left, right, middle element dealloc
        std::vector<std::tuple<int, int>>{{0, 1}, {2, 2}, {1, 1}},
        // in order deallocattion left to right
        std::vector<std::tuple<int, int>>{{0, 1}, {1, 1}, {2, 1}},
        // in order deallocation right to left
        std::vector<std::tuple<int, int>>{{2, 1}, {1, 1}, {0, 1}},
        // deallocation middle, left, right
        std::vector<std::tuple<int, int>>{{1, 1}, {0, 1}, {2, 1}},
        // deallocation middle, right, left
        std::vector<std::tuple<int, int>>{{1, 1}, {2, 1}, {0, 1}}));
