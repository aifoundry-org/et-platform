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
class TestMemcpy : public RuntimeFixture {
public:
  void SetUp() override {
    RuntimeFixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
  }
};
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

// Load and removal of a single kernel.
TEST_F(TestMemcpy, SimpleMemcpyDmaBuffer) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  auto numElems = 1024 * 1024 * 10U;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = runtime_->allocateDmaBuffer(dev, numElems * sizeof(int), true);

  auto ptr = reinterpret_cast<int*>(random_trash->getPtr());
  for (auto i = 0U; i < numElems; ++i) {
    ptr[i] = dis(gen);
  }

  // alloc memory in device
  auto sizeBytes = random_trash->getSize();
  auto d_buffer = runtime_->mallocDevice(dev, sizeBytes);

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, random_trash.get(), d_buffer, sizeBytes);
  auto result = runtime_->allocateDmaBuffer(dev, numElems * sizeof(int), true);
  ASSERT_NE(random_trash, result);

  runtime_->memcpyDeviceToHost(stream, d_buffer, result.get(), sizeBytes);
  runtime_->waitForStream(stream);

  // check only the bytes copied, a dmabuffer could hold larger buffers (in that case there would be random trash at the
  // end)
  auto expectedPtr = reinterpret_cast<int*>(random_trash->getPtr());
  auto actualResult = reinterpret_cast<int*>(result->getPtr());
  for (auto i = 0U; i < numElems; ++i) {
    ASSERT_EQ(expectedPtr[i], actualResult[i]) << "value number " << i << " is not equal.";
  }
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
  auto dmaInfo = deviceLayer_->getDmaInfo(static_cast<int>(dev));
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

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
