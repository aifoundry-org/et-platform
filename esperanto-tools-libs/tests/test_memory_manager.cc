//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Core/MemoryAllocator.h"
#include "Core/MemoryManager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>

using namespace et_runtime::device;

class FakeDevice : public et_runtime::Device {
public:
  FakeDevice() : Device(0) {}
};

TEST(MemoryRange, test_comparison) {
  MemoryRange a = {10, 10};
  // b is right of a
  MemoryRange b = {20, 2};
  EXPECT_TRUE(a < b);

  b = {21, 2};
  EXPECT_TRUE(a < b);

  // b overlaps with a
  b = {12, 3};
  EXPECT_FALSE(a < b);

  // b is left of a
  b = {1, 4};
  EXPECT_FALSE(a < b);
}

TEST(MemoryRange, test_container) {
  std::set<MemoryRange> mem;

  auto res_ins = mem.insert({10, 20});
  ASSERT_TRUE(res_ins.second);

  res_ins = mem.insert({10, 20});
  ASSERT_FALSE(res_ins.second);

  res_ins = mem.insert({11, 2});
  ASSERT_FALSE(res_ins.second);

  res_ins = mem.insert({30, 2});
  ASSERT_TRUE(res_ins.second);

  auto end = mem.rbegin();
  ASSERT_EQ(end->addr_, 30);
}

TEST(MemoryManager, alloc_host) {
  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("fake_device"));
  FakeDevice dev;
  auto mem_manager = MemoryManager(dev);
  uint8_t *ptr = nullptr;
  auto res = mem_manager.mallocHost((void **)&ptr, 10);
  ASSERT_TRUE(ptr != nullptr);
  ASSERT_EQ(res, etrtSuccess);
  res = mem_manager.freeHost(ptr);
  ASSERT_EQ(res, etrtSuccess);
}
