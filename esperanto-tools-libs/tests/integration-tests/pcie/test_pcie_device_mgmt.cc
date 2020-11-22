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
  void SetUp() override { dev_ = std::make_shared<PCIeDevice>(0, true); }

  void TearDown() override { dev_.reset(); }
  std::shared_ptr<PCIeDevice> dev_;
};

TEST_F(PCIEDevTest, FWBase) {
  uint64_t addr = dev_->FWBaseAddr();
  std::cout << "Actual  : " << std::hex << addr << std::endl;
  std::cout << "Expected: " << std::hex << FW_UPDATE_REGION_BEGIN << std::endl;
  ASSERT_EQ(addr, FW_UPDATE_REGION_BEGIN);
}

TEST_F(PCIEDevTest, FWSize) {
  uint32_t size = dev_->FWSize();
  std::cout << "Actual  : " << std::hex << size << std::endl;
  std::cout << "Expected: " << std::hex << FW_UPDATE_REGION_SIZE << std::endl;
  ASSERT_EQ(size, FW_UPDATE_REGION_SIZE);
}

TEST_F(PCIEDevTest, FWMboxMsgMaxSize) {
  auto max_size = dev_->mboxMsgMaxSize();
  ASSERT_EQ(max_size, 2027);
}

TEST_F(PCIEDevTest, FWInit) {
  auto ready = dev_->init();
  ASSERT_TRUE(ready);
}

TEST_F(PCIEDevTest, FWSingleReadMMIO) {
  std::array<uint8_t, 4> data;
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->readDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, FWSingleWriteReadMMIO) {
  std::vector<uint8_t> data = {1, 2, 3, 4};
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::array<uint8_t, 4> data_res;
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}

TEST_F(PCIEDevTest, FWReadWriteMMIO_1k) {
  ssize_t size = 1 << 10;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}

TEST_F(PCIEDevTest, FWReadWriteMMIO_2k) {
  ssize_t size = 2 << 10;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}

TEST_F(PCIEDevTest, FWSingleReadDMA) {
  static constexpr uint32_t size = 1 << 20; // 1MB
  std::array<uint8_t, size> data;
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->readDevMemDMA(addr, data.size(), data.data());
  ASSERT_TRUE(res);
}

TEST_F(PCIEDevTest, FWSingleWriteReadDMA) {
  ssize_t size = 1 << 20;
  std::random_device
      rd; // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(0, std::numeric_limits<uint8_t>::max());

  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
    data[i] = dis(gen);
  }

  uintptr_t addr = dev_->FWBaseAddr();

  auto res = dev_->writeDevMemDMA(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemDMA(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}
/*
// This test takes ~1h ... only if you dare
TEST_F(PCIEDevTest, FWReadWriteMMIO_4M) {
  ssize_t size = 4 << 20; // 4MB
  std::vector<uint8_t> data(size);
  for (ssize_t i; i < size; i++) {
      data[i] = i % 2;
  }
  uintptr_t addr = dev_->FWBaseAddr();
  auto res = dev_->writeDevMemMMIO(addr, data.size(), data.data());
  ASSERT_TRUE(res);
  std::vector<uint8_t> data_res(size);
  res = dev_->readDevMemMMIO(addr, data_res.size(), data_res.data());
  ASSERT_TRUE(res);
  ASSERT_THAT(data_res, ::testing::ElementsAreArray(data));
}*/
