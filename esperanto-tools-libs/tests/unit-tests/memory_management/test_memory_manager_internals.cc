//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagement/BaseMemoryAllocator.h"
#include "MemoryManagement/BidirectionalAllocator.h"
#include "MemoryManagement/LinearAllocator.h"
#include "MemoryManagement/MemoryManagerInternals.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace et_runtime::device::memory_management;

class TestMemoryManagerInternals : public ::testing::Test {

public:
  std::unique_ptr<MemoryManagerInternals> allocator;
};

TEST_F(TestMemoryManagerInternals, allocate_network_tensors) {
  BufferSizeTy kernel_size = 10;
  auto code_md_size = BaseMemoryAllocator::mdSize(BufferType::Code);
  auto code_additional_free = 100;
  BufferSizeTy code_size = kernel_size + code_md_size + code_additional_free;
  auto constant_tensor1_size = 100, constant_tensor2_size = 300;
  auto constant_md_size = BaseMemoryAllocator::mdSize(BufferType::Constant);
  auto activation_size = 10;
  auto placeholder_md_size =
      BaseMemoryAllocator::mdSize(BufferType::Placeholder);
  auto data_additional_free = 30;
  auto data_size = constant_tensor1_size + constant_tensor2_size +
                   2 * constant_md_size + activation_size +
                   placeholder_md_size + data_additional_free;

  allocator.reset(new MemoryManagerInternals(code_size, data_size));
  ASSERT_EQ(allocator->freeMemory(), code_size + data_size);
  // Allocate the network
  auto malloc_res = allocator->mallocCode(kernel_size, 0);
  ASSERT_TRUE((bool)malloc_res);
  auto code_tid = malloc_res.get();
  malloc_res = allocator->mallocConstant(constant_tensor1_size, 0);
  ASSERT_TRUE((bool)malloc_res);
  auto c1_tid = malloc_res.get();
  malloc_res = allocator->mallocConstant(constant_tensor2_size, 0);
  ASSERT_TRUE((bool)malloc_res);
  auto c2_tid = malloc_res.get();
  malloc_res = allocator->mallocPlaceholder(activation_size, 0);
  ASSERT_TRUE((bool)malloc_res);
  auto plc_tid = malloc_res.get();

  ASSERT_EQ(allocator->freeMemory(),
            code_additional_free + data_additional_free);

  // deallocate the memory
  auto free_res = allocator->freeCode(code_tid);
  ASSERT_EQ(free_res, etrtSuccess);
  free_res = allocator->freeData(c1_tid);
  ASSERT_EQ(free_res, etrtSuccess);
  free_res = allocator->freeData(c2_tid);
  ASSERT_EQ(free_res, etrtSuccess);
  free_res = allocator->freeData(plc_tid);
  ASSERT_EQ(free_res, etrtSuccess);

  ASSERT_EQ(allocator->freeMemory(), code_size + data_size);
}
