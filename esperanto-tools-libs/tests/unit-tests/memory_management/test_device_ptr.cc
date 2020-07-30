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

class MockMemoryManager : public MemoryManager {
public:
  MockMemoryManager(Device *dev) : MemoryManager(dev) { initMemRegions(); }

  MOCK_METHOD1(freeData, etrtError(BufferID bid));
  MOCK_METHOD1(freeCode, etrtError(BufferID bid));

  decltype(auto) data_deallocator() { return &data_deallocator_; }
  decltype(auto) code_deallocator() { return &code_deallocator_; }
};

class TestDeviceBuffer : public ::testing::Test {
public:
  TestDeviceBuffer() {
    dev_ = std::make_unique<MockDevice>();
    mem_manager_ = std::make_unique<MockMemoryManager>(dev_.get());
  }

  std::unique_ptr<MockDevice> dev_;
  std::unique_ptr<MockMemoryManager> mem_manager_;
};

TEST_F(TestDeviceBuffer, dealloc_data) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(1);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  ASSERT_EQ(buf.ref_cntr(), 1);
}

TEST_F(TestDeviceBuffer, dealloc_code) {
  EXPECT_CALL(*mem_manager_, freeCode(testing::_)).Times(1);
  DeviceBuffer buf(1, 100, 10, mem_manager_->code_deallocator());
  ASSERT_EQ(buf.ref_cntr(), 1);
}

TEST_F(TestDeviceBuffer, copy_constr) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(1);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());

  {
    DeviceBuffer buf2(buf);
    ASSERT_TRUE(!buf.unique());
    ASSERT_EQ(buf.ref_cntr(), 2);

    DeviceBuffer buf3 = buf;
    ASSERT_EQ(buf.ref_cntr(), 3);

    ASSERT_TRUE(buf.id() == buf2.id() == buf3.id());
  }
  ASSERT_EQ(buf.ref_cntr(), 1);
}

TEST_F(TestDeviceBuffer, assignment_op) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(2);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  DeviceBuffer buf2(1, 100, 10, mem_manager_->data_deallocator());

  buf2 = buf;
}

TEST_F(TestDeviceBuffer, move_constr) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(1);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  DeviceBuffer buf2(std::move(buf));

  EXPECT_EQ(buf2.ref_cntr(), 1);
  EXPECT_EQ(buf.ref_cntr(), 0);
}

TEST_F(TestDeviceBuffer, move_op) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(1);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  DeviceBuffer buf2;

  buf2 = std::move(buf);

  EXPECT_EQ(buf2.ref_cntr(), 1);
  EXPECT_EQ(buf.ref_cntr(), 0);
}

TEST_F(TestDeviceBuffer, containers_map) {
  std::map<DeviceBuffer, int> dev_map;
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(2);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  DeviceBuffer buf2(2, 100, 10, mem_manager_->data_deallocator());

  dev_map[buf] = 1;
  dev_map[buf2] = 2;
}

TEST_F(TestDeviceBuffer, containers_unordered_map) {
  std::unordered_map<DeviceBuffer, int> dev_map;
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(2);
  DeviceBuffer buf(1, 100, 10, mem_manager_->data_deallocator());
  DeviceBuffer buf2(2, 100, 10, mem_manager_->data_deallocator());

  dev_map[buf] = 1;
  dev_map[buf2] = 2;
}

TEST_F(TestDeviceBuffer, add_op) {
  EXPECT_CALL(*mem_manager_, freeData(testing::_)).Times(1);
  auto base_offset = 100;
  auto size = 20;
  DeviceBuffer buf(1, base_offset, size, mem_manager_->data_deallocator());

  auto buf2 = buf + 10;
  ASSERT_EQ(reinterpret_cast<BufferOffsetTy>(buf2.ptr()), base_offset + 10);
  ASSERT_EQ(buf2.size(), size - 10);

  auto buf3 = 20 + buf2;
  ASSERT_EQ(reinterpret_cast<BufferOffsetTy>(buf3.ptr()), base_offset + 30);
  ASSERT_EQ(buf3.size(), size - 30);
}
