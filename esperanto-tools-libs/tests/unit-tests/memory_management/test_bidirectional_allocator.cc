//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagement/BidirectionalAllocator.h"

#include "esperanto/runtime/Core/CommandLineOptions.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace et_runtime::device::memory_management;

class TestBidirectionalAllocator : public ::testing::Test {

public:
  std::unique_ptr<BidirectionalAllocator> allocator;

  std::list<BufferInfo<FreeRegion>> &free_list() {
    return allocator->free_list_;
  }

  std::list<std::shared_ptr<AbstractBufferInfo>> &allocated_front_list() {
    return allocator->allocated_front_list_;
  }

  std::list<std::shared_ptr<AbstractBufferInfo>> &allocated_back_list() {
    return allocator->allocated_back_list_;
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

// Expect that this mallocFront will fail because we do not have space for the
// tensor meta-data
TEST_F(TestBidirectionalAllocator, mallocFront_fail_md_size) {
  allocator.reset(new BidirectionalAllocator(100, 100));
  auto res = allocator->mallocFront(BufferType::Code, 100, 0);

  ASSERT_FALSE((bool)res);
}

// Single memory allocations
TEST_F(TestBidirectionalAllocator, single_mallocFront_success) {
  auto base_start = 100;
  auto type = BufferType::Code;
  auto buffer_size = 10;
  auto total_size = buffer_size + allocator->mdSize(type);

  allocator.reset(new BidirectionalAllocator(base_start, total_size));
  auto res = allocator->mallocFront(type, buffer_size, 0);

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
  ASSERT_TRUE(allocated_front_list().empty());
}

// One memory allocation from each direction, and reverse order frees
TEST_F(TestBidirectionalAllocator, mallocs_both_directions) {

  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  BufferSizeTy tensor_size = 20;
  BufferOffsetTy dram_base = 100;
  BufferSizeTy free_buffer = 20;
  BufferSizeTy free_size_aux = 30;
  auto dram_size = 2 * (tensor_size + md_size) + free_size_aux;

  allocator.reset(new BidirectionalAllocator(dram_base, dram_size));

  auto res = allocator->mallocFront(type, tensor_size, 0);
  ASSERT_TRUE((bool)res);

  // firs tensor id
  auto tid_1 = res.get();

  res = allocator->mallocBack(type, tensor_size, 0);
  ASSERT_TRUE((bool)res);
  auto tid_2 = res.get();

  allocator->printStateJSON();

  // The list should be empty now
  ASSERT_EQ(allocator->freeMemory(), free_size_aux);

  auto free_res = allocator->free(tid_1);
  ASSERT_EQ(free_res, etrtSuccess);

  ASSERT_EQ(allocator->freeMemory(), dram_size - (tensor_size + md_size));

  free_res = allocator->free(tid_2);
  ASSERT_EQ(free_res, etrtSuccess);

  ASSERT_EQ(free_list().size(), 1);
  ASSERT_TRUE(allocated_front_list().empty());
  ASSERT_TRUE(allocated_back_list().empty());
}

// parametrized test class, receives as parameters the direction of allocation
class TestBidirectionalAllocatorMallocAlignmentDirections
    : public TestBidirectionalAllocator,
      public testing::WithParamInterface<
          BidirectionalAllocator::AllocDirection> {};

// Multiple allocations
TEST_P(TestBidirectionalAllocatorMallocAlignmentDirections,
       malloc_alignment_success) {

  auto direction = GetParam();
  // Function pointer to the malloc function to execute: either mallocFront or
  // malloc Back
  et_runtime::ErrorOr<et_runtime::BufferID> (
      BidirectionalAllocator::*mallocFunc)(BufferType type, BufferSizeTy size,
                                           BufferSizeTy alignment);

  // Function that returns tensors allocated alignedStart offset
  std::function<BufferOffsetTy(BufferSizeTy size)> tensor_aligned_start;

  // helper that returns the allocation list
  std::function<std::shared_ptr<AbstractBufferInfo> &()> last_tensor_allocated;

  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  BufferSizeTy tensor_size = 20;
  BufferOffsetTy dram_base = 100;
  auto max_alignment = 1ULL << 14; // 16KB
  auto dram_size = tensor_size + md_size + max_alignment;

  if (direction == BidirectionalAllocator::AllocDirection::Front) {
    mallocFunc = &BidirectionalAllocator::mallocFront;
    last_tensor_allocated = [this]() -> std::shared_ptr<AbstractBufferInfo> & {
      return allocated_front_list().back();
    };

  } else {
    mallocFunc = &BidirectionalAllocator::mallocBack;
    last_tensor_allocated = [this]() -> std::shared_ptr<AbstractBufferInfo> & {
      return allocated_back_list().front();
    };
  }

  allocator.reset(new BidirectionalAllocator(dram_base, dram_size));

  for (int i = 0; i < 14; i++) {
    auto alignment = 1ULL << i;
    auto res = ((*allocator).*mallocFunc)(type, tensor_size, alignment);

    ASSERT_TRUE((bool)res);

    allocator->printState();

    // We expect to have allocated the first tensor
    auto tid = res.get();

    auto tensor = last_tensor_allocated();
    ASSERT_EQ(tensor->alignedStart() % alignment, 0);

    auto free_res = allocator->free(tid);
    ASSERT_TRUE((bool)res);
    ASSERT_EQ(free_list().size(), 1);
    auto free_entry = free_list().front();

    ASSERT_EQ(free_entry.base(), dram_base);
    ASSERT_EQ(free_entry.size(), dram_size);
  }
}

INSTANTIATE_TEST_CASE_P(
    MultipleMallocs, TestBidirectionalAllocatorMallocAlignmentDirections,
    testing::Values(BidirectionalAllocator::AllocDirection::Front,
                    BidirectionalAllocator::AllocDirection::Back));

// parametrized test class, receives as parameters the direction of allocation
class TestBidirectionalAllocatorMallocDirections
    : public TestBidirectionalAllocator,
      public testing::WithParamInterface<
          BidirectionalAllocator::AllocDirection> {};

// Multiple allocations
TEST_P(TestBidirectionalAllocatorMallocDirections, multiple_mallocs_success) {

  auto direction = GetParam();
  // Function pointer to the malloc function to execute: either mallocFront or
  // malloc Back
  et_runtime::ErrorOr<et_runtime::BufferID> (
      BidirectionalAllocator::*mallocFunc)(BufferType type, BufferSizeTy size,
                                           BufferSizeTy alignment);

  // Function that computes the begining address
  // of the free list based on the allocation direction
  std::function<BufferOffsetTy(BufferSizeTy size)> free_list_start;

  // Function that computes the begining address
  // of the tensor based on the allocation direction
  std::function<BufferOffsetTy(BufferSizeTy size)> tensor_start;

  // helper that returns the allocation list
  std::function<std::shared_ptr<AbstractBufferInfo> &()> last_tensor_allocated;

  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  BufferSizeTy tensor_size = 20;
  BufferOffsetTy dram_base = 100;
  auto dram_size = 3 * (tensor_size + md_size);

  if (direction == BidirectionalAllocator::AllocDirection::Front) {
    mallocFunc = &BidirectionalAllocator::mallocFront;
    last_tensor_allocated = [this]() -> std::shared_ptr<AbstractBufferInfo> & {
      return allocated_front_list().back();
    };
    free_list_start = [dram_base](BufferSizeTy size) -> BufferOffsetTy {
      return dram_base + size;
    };
    tensor_start = [dram_base, tensor_size](
                       BufferSizeTy allocated_size) -> BufferOffsetTy {
      return dram_base + allocated_size - tensor_size;
    };
  } else {
    mallocFunc = &BidirectionalAllocator::mallocBack;
    last_tensor_allocated = [this]() -> std::shared_ptr<AbstractBufferInfo> & {
      return allocated_back_list().front();
    };
    // the free list base remains the beginning of the region
    free_list_start = [dram_base](BufferSizeTy size) -> BufferOffsetTy {
      return dram_base;
    };
    tensor_start = [dram_base, dram_size,
                    md_size](BufferSizeTy allocated_size) -> BufferOffsetTy {
      return dram_base + dram_size - (allocated_size - md_size);
    };
  }

  allocator.reset(new BidirectionalAllocator(dram_base, dram_size));

  ASSERT_EQ(free_list().front().size(), dram_size);
  /// TODO check the double linked list meta-data

  BufferSizeTy allocated_size = 0;

  // First allocation
  {
    auto res = ((*allocator).*mallocFunc)(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    allocated_size += tensor_size + md_size;
    ASSERT_EQ(free_list().front().base(), free_list_start(allocated_size));
    ASSERT_EQ(free_list().front().size(), dram_size - allocated_size);

    auto tensor = last_tensor_allocated();
    ASSERT_EQ(tensor->alignedStart(), tensor_start(allocated_size));
  }

  // Second allocation
  {
    auto res = ((*allocator).*mallocFunc)(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    allocated_size += tensor_size + md_size;
    ASSERT_EQ(free_list().front().base(), free_list_start(allocated_size));
    ASSERT_EQ(free_list().front().size(), dram_size - allocated_size);

    auto tensor = last_tensor_allocated();
    ASSERT_EQ(tensor->alignedStart(), tensor_start(allocated_size));
  }

  // Third allocation
  {
    auto res = ((*allocator).*mallocFunc)(type, tensor_size, 0);
    ASSERT_TRUE((bool)res);
    ASSERT_TRUE(free_list().empty());
    allocated_size += tensor_size + md_size;
    auto tensor = last_tensor_allocated();
    ASSERT_EQ(tensor->alignedStart(), tensor_start(allocated_size));
  }
}

INSTANTIATE_TEST_CASE_P(
    MultipleMallocs, TestBidirectionalAllocatorMallocDirections,
    testing::Values(BidirectionalAllocator::AllocDirection::Front,
                    BidirectionalAllocator::AllocDirection::Back));

// parametried test class, receives as parameters the allocation direciton and a
// vector of tuples that  represent the order of deallocation and expected
// free-list size
class TestBidirectionalAllocatorMallocFreeOrder
    : public TestBidirectionalAllocator,
      public testing::WithParamInterface<
          std::tuple<BidirectionalAllocator::AllocDirection,
                     std::vector<std::tuple<int, int>>>> {};

// Multiple allocations and frees, change the deallocation order
// The vector of tuples has pas parameters <tensor_pos, free_list_size>
// We are modifying the order by which we are de-allocating tesnors and
// we check the number of free-list regions that are being created.
// Based on the order of deallocation adjacent regions that get freed
// shold be merged into a larger region. At the end we should have the original
// total free space
// In this test no 'gap" is allowed betwen the tensors
TEST_P(TestBidirectionalAllocatorMallocFreeOrder, mallocFront_free_random) {
  auto &[alloc_direction, tensor_free_order] = GetParam();
  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  std::vector<BufferSizeTy> tensor_size = {20, 30, 40};
  std::vector<et_runtime::BufferID> tensors = {};
  BufferOffsetTy dram_base = 0;
  auto dram_size = 0;
  for (auto &i : tensor_size) {
    dram_size += i + md_size;
  }

  allocator.reset(new BidirectionalAllocator(dram_base, dram_size));

  for (auto size : tensor_size) {
    et_runtime::BufferID tid;
    if (alloc_direction == BidirectionalAllocator::AllocDirection::Front) {
      auto res = allocator->mallocFront(type, size, 0);
      ASSERT_TRUE((bool)res);
      tid = res.get();
    } else {
      auto res = allocator->mallocBack(type, size, 0);
      ASSERT_TRUE((bool)res);
      tid = res.get();
    }
    tensors.push_back(tid);
  }

  // allocator->printState();

  ASSERT_TRUE(free_list().empty());

  auto total_free_size = allocator->freeMemory();

  for (auto i : tensor_free_order) {
    auto &[tensor, free_list_size] = i;
    auto tid = tensors[tensor];
    auto t_ptr = findAllocatedBuffer(tid);
    assert(t_ptr != nullptr);
    total_free_size += t_ptr->size();
    auto free_res = allocator->free(tid);
    ASSERT_EQ(free_res, etrtSuccess);
    ASSERT_EQ(free_list().size(), free_list_size);
    ASSERT_EQ(total_free_size, allocator->freeMemory());
    // allocator->printState();
  }
  ASSERT_EQ(free_list().front().size(), dram_size);
}

INSTANTIATE_TEST_CASE_P(
    MallocFrontFree, TestBidirectionalAllocatorMallocFreeOrder,
    testing::Values(
        // left, right, middle element dealloc
        std::make_tuple(BidirectionalAllocator::AllocDirection::Front,
                        std::vector<std::tuple<int, int>>{
                            {0, 1}, {2, 2}, {1, 1}}),
        // in order deallocattion left to right
        std::make_tuple(BidirectionalAllocator::AllocDirection::Front,
                        std::vector<std::tuple<int, int>>{
                            {0, 1}, {1, 1}, {2, 1}}),
        // in order deallocation right to left
        std::make_tuple(BidirectionalAllocator::AllocDirection::Front,
                        std::vector<std::tuple<int, int>>{
                            {2, 1}, {1, 1}, {0, 1}}),
        // deallocation middle, left, right
        std::make_tuple(BidirectionalAllocator::AllocDirection::Front,
                        std::vector<std::tuple<int, int>>{
                            {1, 1}, {0, 1}, {2, 1}}),
        // deallocation middle, right, left
        std::make_tuple(BidirectionalAllocator::AllocDirection::Front,
                        std::vector<std::tuple<int, int>>{
                            {1, 1}, {2, 1}, {0, 1}}),
        // Same list as above only changed the direction of allocation
        std::make_tuple(BidirectionalAllocator::AllocDirection::Back,
                        std::vector<std::tuple<int, int>>{
                            {0, 1}, {2, 2}, {1, 1}}),
        std::make_tuple(BidirectionalAllocator::AllocDirection::Back,
                        std::vector<std::tuple<int, int>>{
                            {0, 1}, {1, 1}, {2, 1}}),
        std::make_tuple(BidirectionalAllocator::AllocDirection::Back,
                        std::vector<std::tuple<int, int>>{
                            {2, 1}, {1, 1}, {0, 1}}),
        std::make_tuple(BidirectionalAllocator::AllocDirection::Back,
                        std::vector<std::tuple<int, int>>{
                            {1, 1}, {0, 1}, {2, 1}}),
        std::make_tuple(BidirectionalAllocator::AllocDirection::Back,
                        std::vector<std::tuple<int, int>>{
                            {1, 1}, {2, 1}, {0, 1}})));

// parametried test class, receives as parameters the allocation direciton and a
// vector of tuples that  represent the order of deallocation and expected
// free-list size
class TestBidirectionalAllocatorMallocFreeAlterOrder
    : public TestBidirectionalAllocator,
      public testing::WithParamInterface<
          std::tuple<std::vector<BidirectionalAllocator::AllocDirection>,
                     std::vector<int>>> {};

TEST_P(TestBidirectionalAllocatorMallocFreeAlterOrder,
       malloc_front_back_free_random) {
  auto &[alloc_direction, tensor_free_order] = GetParam();
  ASSERT_EQ(alloc_direction.size(), tensor_free_order.size());
  auto type = BufferType::Code;
  auto md_size = allocator->mdSize(type);
  std::vector<std::tuple<BufferSizeTy, BidirectionalAllocator::AllocDirection>>
      tensor_info;
  std::vector<et_runtime::BufferID> tensors = {};
  BufferOffsetTy dram_base = 0;
  auto dram_size = 0;
  int min_tensor_size = 10;
  for (int i = 0; i < alloc_direction.size(); i++) {
    auto tsize = min_tensor_size * (i + 1);
    tensor_info.push_back({tsize, alloc_direction[i]});
    dram_size += tsize + md_size;
  }

  allocator.reset(new BidirectionalAllocator(dram_base, dram_size));

  for (auto [size, alloc_direction] : tensor_info) {
    et_runtime::BufferID tid;
    if (alloc_direction == BidirectionalAllocator::AllocDirection::Front) {
      auto res = allocator->mallocFront(type, size, 0);
      ASSERT_TRUE((bool)res);
      tid = res.get();
    } else {
      auto res = allocator->mallocBack(type, size, 0);
      ASSERT_TRUE((bool)res);
      tid = res.get();
    }
    tensors.push_back(tid);
  }

  // allocator->printState();

  ASSERT_TRUE(free_list().empty());

  auto total_free_size = allocator->freeMemory();

  for (auto tensor : tensor_free_order) {
    assert(tensor < tensors.size());
    auto tid = tensors[tensor];
    auto t_ptr = findAllocatedBuffer(tid);
    assert(t_ptr != nullptr);
    total_free_size += t_ptr->size();
    auto free_res = allocator->free(tid);
    ASSERT_EQ(free_res, etrtSuccess);
    ASSERT_EQ(total_free_size, allocator->freeMemory());
    // allocator->printState();
  }
  ASSERT_EQ(free_list().front().size(), dram_size);
}

INSTANTIATE_TEST_CASE_P(
    MallocFrontBackFree, TestBidirectionalAllocatorMallocFreeAlterOrder,
    testing::Values(
        // left, right, middle element dealloc
        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
            },

            std::vector<int>{0, 1, 2, 3}),
        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Back,
            },
            std::vector<int>{0, 1, 2, 3}),
        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Front,
            },
            std::vector<int>{0, 1, 2, 3}),
        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
            },
            std::vector<int>{0, 2, 1, 3}),

        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
            },
            std::vector<int>{3, 2, 1, 0}),

        std::make_tuple(
            std::vector<BidirectionalAllocator::AllocDirection>{
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
                BidirectionalAllocator::AllocDirection::Front,
                BidirectionalAllocator::AllocDirection::Back,
            },
            std::vector<int>{1, 0, 3, 2})));
