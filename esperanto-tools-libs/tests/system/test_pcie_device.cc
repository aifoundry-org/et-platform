//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <Device/PCIeDevice.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <thread>

using namespace et_runtime::device;

class PCIEDevTest : public ::testing::Test {
protected:
  void SetUp() override { dev_ = std::make_shared<PCIeDevice>(0); }

  void TearDown() override { dev_.reset(); }
  std::shared_ptr<PCIeDevice> dev_;
};

TEST_F(PCIEDevTest, SingleWriteMMIO) {
  std::vector<uint8_t> data = {1, 2, 3, 4};
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->writeDevMem(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, SingleReadMMIO) {
  std::array<uint8_t, 4> data;
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->readDevMem(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, SingleWriteReadMMIO) {
  std::vector<uint8_t> data = {1, 2, 3, 4};
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->writeDevMem(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::array<uint8_t, 4> data_res;
  res = dev_->readDevMem(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}


/* FIXME Disable test for now
TEST_F(PCIEDevTest, ReadWriteMMIO_8k) {
  ssize_t size = 1 << 8 * 1 << 10;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }
  auto res = dev_->writeDevMem(0xdeadbeef, data.size(), data.data());
  ASSERT_TRUE(res);
  std::array<uint8_t, 4> data_res;
  res = dev_->readDevMem(0xdeadbeef, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}
*/
