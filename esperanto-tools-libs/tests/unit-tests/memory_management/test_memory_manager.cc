//******************************************************************************
// Copyright (C) 2020,, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/Memory.h"
#include "esperanto/runtime/Core/MemoryManager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

using namespace et_runtime;
using namespace et_runtime::device;

TEST(RefCount, simple) {
  RefCounter ctr;

  ASSERT_EQ(ctr.val(), 0);
  auto res = ctr.inc();
  ASSERT_EQ(res, 1);
  res = ctr.dec();
  ASSERT_EQ(res, 0);
}

class MockDevice : public Device {
public:
  MockDevice() : Device() {}

  uintptr_t dramSize() const override { return 1ULL << 33; }
  uintptr_t dramBaseAddr() const override { return 0; }
};

class TestMemoryManager : public ::testing::Test {
public:
  TestMemoryManager() {
    dev_ = std::make_unique<MockDevice>();
    mem_manager_ = std::make_unique<MemoryManager>(dev_.get());
    mem_manager_->initMemRegions();
  }

  decltype(auto) mallocCode(size_t size, const BufferDebugInfo &info) {
    return mem_manager_->mallocCode(size, info);
  }

  decltype(auto) mallocConstant(size_t size, const BufferDebugInfo &info) {
    return mem_manager_->mallocConstant(size, info);
  }

  decltype(auto) mallocPlaceholder(size_t size, const BufferDebugInfo &info) {
    return mem_manager_->mallocPlaceholder(size, info);
  }

  std::unique_ptr<MockDevice> dev_;
  std::unique_ptr<MemoryManager> mem_manager_;
};

TEST_F(TestMemoryManager, malloc_code) {
  BufferDebugInfo info = {1, 1, 1};
  auto res = mallocCode(10, info);
  ASSERT_TRUE((bool)res);
}

TEST_F(TestMemoryManager, malloc_constant) {
  BufferDebugInfo info = {1, 1, 1};
  auto res = mallocConstant(10, info);
  ASSERT_TRUE((bool)res);
}

TEST_F(TestMemoryManager, malloc_placeholders) {
  BufferDebugInfo info = {1, 1, 1};
  auto res = mallocPlaceholder(10, info);
  ASSERT_TRUE((bool)res);
}
