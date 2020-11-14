//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "PCIEDevice/PCIeDevice.h"

#include <esperanto-fw/firmware_helpers/layout.h>

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

TEST_F(PCIEDevTest, DramBase) {
  auto base_addr = dev_->dramBaseAddr();
  // DRAM region is 512GB starting at 0x8000000000
  ASSERT_TRUE(base_addr >= 0x8000000000ULL && base_addr <= 0xFFFFFFFFFFULL);
}

TEST_F(PCIEDevTest, DramSize) {
  auto size = dev_->dramSize();
  // DRAM region is 512GB
  ASSERT_TRUE(size > 0 && size <= 512ULL * 1024 * 1024 * 1024);
}

/* Disabling since VQ is using part of the Mbox - till we migrate to DDR
TEST_F(PCIEDevTest, MboxMsgMaxSize) {
  auto max_size = dev_->mboxMsgMaxSize();
  ASSERT_EQ(max_size, 2027);
}
*/

TEST_F(PCIEDevTest, Init) {
  auto ready = dev_->init();
  ASSERT_TRUE(ready);
}

TEST_F(PCIEDevTest, SingleReadMMIO) {
  std::array<uint8_t, 4> data;
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->readDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, SingleWriteReadMMIO) {
  std::vector<uint8_t> data = {1, 2, 3, 4};
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::array<uint8_t, 4> data_res;
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}


TEST_F(PCIEDevTest, ReadWriteMMIO_1k) {
  ssize_t size = 1 << 10;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }
  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}

TEST_F(PCIEDevTest, SingleReadDMA) {
  static constexpr uint32_t size = 1 << 20; // 1MB
  std::array<uint8_t, size> data;
  uintptr_t addr = dev_->dramBaseAddr();
  auto res = dev_->readDevMemDMA(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, SingleWriteReadDMA) {
  ssize_t size = 1 << 20;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }

  uintptr_t addr = dev_->dramBaseAddr() + 0xdeadbeef;

  auto res = dev_->writeDevMemDMA(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemDMA(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}
