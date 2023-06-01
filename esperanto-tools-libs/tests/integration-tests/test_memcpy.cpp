//******************************************************************************
// Copyright (C) 2020,2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "RuntimeImp.h"
#include "common/Constants.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <random>

namespace {
class TestMemcpy : public RuntimeFixture {};
} // namespace

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

TEST_F(TestMemcpy, 2GbMemcpy) {
  using ValueType = uint32_t;
  std::mt19937 gen(std::random_device{}());

  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto desiredSize = 1ULL << 31;

  struct RandomData {
    ValueType value;
    ValueType position;
  };
  auto numValues = 500U;
  std::vector<RandomData> rd;
  // alloc memory in device
  auto d_buffer = runtime_->mallocDevice(dev, desiredSize);
  std::vector<ValueType> h_buffer(desiredSize / sizeof(ValueType));
  std::uniform_int_distribution<ValueType> dis(0, static_cast<ValueType>(h_buffer.size() - 1));

  for (auto i = 0U; i < numValues; ++i) {
    auto data = RandomData{dis(gen), dis(gen)};
    rd.emplace_back(data);
  }
  for (auto v : rd) {
    h_buffer[v.position] = v.value;
  }

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(h_buffer.data()), d_buffer, desiredSize);

  // wait for stream to put 0s in the previously filled data before copying from device
  runtime_->waitForStream(stream);

  for (auto v : rd) {
    h_buffer[v.position] = 0U;
  }

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(h_buffer.data()), desiredSize);
  runtime_->waitForStream(stream);

  for (auto v : rd) {
    EXPECT_EQ(h_buffer[v.position], v.value);
  }
}

TEST_F(TestMemcpy, dmaListCheckExceptions) {
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto dmaInfo = runtime_->getDmaInfo(dev);
  rt::MemcpyList list;
  list.addOp(nullptr, nullptr, dmaInfo.maxElementSize_ + 1);
  EXPECT_THROW(runtime_->memcpyHostToDevice(stream, list);, rt::Exception);
  list.operations_.clear();
  for (auto i = 0U; i <= dmaInfo.maxElementCount_; ++i) {
    list.addOp(nullptr, nullptr, 1);
  }
  EXPECT_THROW(runtime_->memcpyHostToDevice(stream, list);, rt::Exception);
}

TEST_F(TestMemcpy, dmaListSimple) {
  if (sRtType == RtType::MP) {
    RT_LOG(INFO)
      << "Skipping this test until SW-13139 is implemented, uncomment the return and change the dmaInfo query";
    return;
  }
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto dmaInfo = deviceLayer_->getDmaInfo(static_cast<int>(dev));
  rt::MemcpyList listH2D;
  rt::MemcpyList listD2H;
  std::vector<std::byte*> deviceMem;
  std::vector<std::vector<std::byte>> hostMemSrc;
  std::vector<std::vector<std::byte>> hostMemDst;
  std::mt19937 gen(std::random_device{}());
  auto maxEntrySize = 1024UL;
  std::uniform_int_distribution dis(1UL, maxEntrySize);
  for (auto i = 0U; i < dmaInfo.maxElementCount_; ++i) {
    auto entrySize = dis(gen);
    deviceMem.emplace_back(runtime_->mallocDevice(dev, entrySize));
    std::vector<std::byte> currentEntry;
    for (auto j = 0U; j < entrySize; ++j) {
      currentEntry.emplace_back(std::byte(dis(gen) % 256));
    }
    hostMemSrc.emplace_back(currentEntry);
    hostMemDst.emplace_back(std::vector<std::byte>(entrySize));
    listH2D.addOp(hostMemSrc[i].data(), deviceMem[i], entrySize);
    listD2H.addOp(deviceMem[i], hostMemDst[i].data(), entrySize);
  }
  // check src and dst are different now
  for (auto i = 0U; i < hostMemSrc.size(); ++i) {
    ASSERT_NE(hostMemSrc[i], hostMemDst[i]);
  }
  runtime_->memcpyHostToDevice(stream, listH2D);
  runtime_->memcpyDeviceToHost(stream, listD2H);
  runtime_->waitForStream(stream);

  // check src and dst are equal after the copies
  for (auto i = 0U; i < hostMemSrc.size(); ++i) {
    ASSERT_EQ(hostMemSrc[i], hostMemDst[i]);
  }
}

TEST_F(TestMemcpy, memcpyD2DCheckExceptions) {
  if (sDlType != RuntimeFixture::DeviceLayerImp::PCIE) { // force multidevice if its not PCIE
    numDevices_ = 2;
    TearDown();
    SetUp();
  }
  ASSERT_GT(devices_.size(), 1);
  auto dev1 = devices_[0];
  auto dev2 = devices_[1];
  if (sDlType != RuntimeFixture::DeviceLayerImp::PCIE) {
    RT_LOG(INFO) << "MemcpyDeviceToDevice only supported in PCIE";
    ASSERT_FALSE(runtime_->isP2PEnabled(dev1, dev2));
    return;
  }
  ASSERT_TRUE(runtime_->isP2PEnabled(dev1, dev2));
  auto dmaInfoMaxSize = runtime_->getDmaInfo(dev1).maxElementSize_;
  auto b1 = runtime_->mallocDevice(dev1, dmaInfoMaxSize + 1);
  auto b2 = runtime_->mallocDevice(dev2, dmaInfoMaxSize * 2 + 1) +
            dmaInfoMaxSize; // this is done to check the buffer address between both is different
  ASSERT_NE(b1, b2);

  // bigger size than supported
  EXPECT_THROW(runtime_->memcpyDeviceToDevice(dev1, defaultStreams_[1], b1, b2, dmaInfoMaxSize + 1), rt::Exception);
  EXPECT_THROW(runtime_->memcpyDeviceToDevice(defaultStreams_[0], dev2, b1, b2, dmaInfoMaxSize + 1), rt::Exception);

  // invalid buffers
  EXPECT_THROW(runtime_->memcpyDeviceToDevice(dev1, defaultStreams_[1], b2, b1, dmaInfoMaxSize), rt::Exception);
  EXPECT_THROW(runtime_->memcpyDeviceToDevice(defaultStreams_[0], dev2, b2, b1, dmaInfoMaxSize), rt::Exception);

  // should work
  EXPECT_NO_THROW(runtime_->memcpyDeviceToDevice(dev1, defaultStreams_[1], b1, b2, dmaInfoMaxSize));
  EXPECT_NO_THROW(runtime_->memcpyDeviceToDevice(defaultStreams_[0], dev2, b1, b2, dmaInfoMaxSize));

  runtime_->waitForStream(defaultStreams_[0]);
  runtime_->waitForStream(defaultStreams_[1]);
}

TEST_F(TestMemcpy, simpleMemcpyD2D) {

  if (sDlType != RuntimeFixture::DeviceLayerImp::PCIE) { // force multidevice if its not PCIE
    numDevices_ = 2;
    TearDown();
    SetUp();
  }

  auto devices = runtime_->getDevices();
  ASSERT_GT(devices.size(), 1);
  auto dev1 = devices_[0];
  auto dev2 = devices_[1];
  if (sDlType != RuntimeFixture::DeviceLayerImp::PCIE) {
    RT_LOG(INFO) << "MemcpyDeviceToDevice only supported in PCIE";
    ASSERT_FALSE(runtime_->isP2PEnabled(dev1, dev2));
    return;
  }
  ASSERT_TRUE(runtime_->isP2PEnabled(dev1, dev2));
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  auto numElems = 1024 * 1024 * 10U;
  auto random_trash = std::vector<int>(numElems);

  for (auto i = 0U; i < numElems; i += static_cast<uint32_t>(rand() % 128)) {
    random_trash[i] = dis(gen);
  }

  // alloc memory in device
  auto sizeBytes = random_trash.size() * sizeof(int);
  auto d_buffer1 = runtime_->mallocDevice(dev1, sizeBytes);
  auto d_buffer2 = runtime_->mallocDevice(dev2, sizeBytes);

  auto streamSrc = defaultStreams_[0];
  auto streamDst = defaultStreams_[1];

  // copy from host to device, then from device to device and finally from device to result buffer host; check they are
  // equal
  runtime_->memcpyHostToDevice(streamSrc, reinterpret_cast<std::byte*>(random_trash.data()), d_buffer1, sizeBytes);
  runtime_->memcpyDeviceToDevice(streamSrc, dev2, d_buffer1, d_buffer2, sizeBytes);

  auto result = std::vector<int>(numElems);
  ASSERT_NE(random_trash, result);

  // need to sync before sending the next memcpyDeviceToHost
  runtime_->waitForStream(streamSrc);
  runtime_->memcpyDeviceToHost(streamDst, d_buffer2, reinterpret_cast<std::byte*>(result.data()), sizeBytes);

  // final sync and check results
  runtime_->waitForStream(streamDst);
  ASSERT_EQ(random_trash, result);
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
