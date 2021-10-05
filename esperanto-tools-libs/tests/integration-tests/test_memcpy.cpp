//******************************************************************************
// Copyright (C) 2020,2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeImp.h"
#include "TestUtils.h"
#include "common/Constants.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <random>

namespace {
class TestMemcpy : public Fixture {
public:
  void SetUp() override {
    Fixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
  }
};

// Load and removal of a single kernel.
TEST_F(TestMemcpy, SimpleMemcpy) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  auto numElems = 1024 * 1024 * 10;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = std::vector<int>();

  for (int i = 0; i < numElems; ++i) {
    random_trash.emplace_back(dis(gen));
  }

  // alloc memory in device
  auto sizeBytes = random_trash.size() * sizeof(int);
  auto d_buffer = runtime_->mallocDevice(dev, sizeBytes);

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(random_trash.data()), d_buffer, sizeBytes);
  auto result = std::vector<int>(static_cast<unsigned long>(numElems));
  ASSERT_NE(random_trash, result);

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(result.data()), sizeBytes);
  runtime_->waitForStream(stream);

  ASSERT_EQ(random_trash, result);
}

TEST_F(TestMemcpy, 4GbMemcpy) {
  using ValueType = uint32_t;
  std::mt19937 gen(std::random_device{}());

  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto desiredSize = 1ULL << 32;

  struct RandomData {
    ValueType value;
    ValueType position;
  };
  auto numValues = 500U;
  std::vector<RandomData> rd;
  // alloc memory in device
  auto d_buffer = runtime_->mallocDevice(dev, desiredSize);
  std::vector<ValueType> h_buffer(desiredSize / sizeof(ValueType));
  std::uniform_int_distribution<ValueType> dis(0, static_cast<ValueType>(h_buffer.size()-1));

  for (auto i = 0U; i < numValues; ++i) {
    auto data = RandomData{dis(gen), dis(gen)};
    rd.emplace_back(data);
  }
  for (auto v : rd) {
    h_buffer[v.position] = v.value;
  }

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(h_buffer.data()), d_buffer, desiredSize);

  //wait for stream to put 0s in the previously filled data before copying from device
  runtime_->waitForStream(stream);

  for (auto v: rd) {
    h_buffer[v.position] = 0U;
  }

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(h_buffer.data()), desiredSize);
  runtime_->waitForStream(stream);

  for (auto v: rd) {
    EXPECT_EQ(h_buffer[v.position], v.value);
  }
}

} // namespace

int main(int argc, char** argv) {
  Fixture::sMode = IsPcie(argc, argv) ? Fixture::Mode::PCIE : Fixture::Mode::SYSEMU;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
