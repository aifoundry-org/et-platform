//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"

#include "TestUtils.h"
#include "common/Constants.h"
#include "utils.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logger.h>

#include <experimental/filesystem>
#include <fstream>
#include <ios>
#include <random>

namespace {

struct FwTracesTest : public Fixture {
  FwTracesTest() {
    auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    init(std::move(deviceLayer));
  }
};
// Load and removal of a single kernel.
TEST_F(FwTracesTest, CM_MM_Traces) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  auto numElems = 1024 * 1024 * 10;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = std::vector<int>();

  // enable device traces
  runtime_->setupDeviceTracing(stream, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
  std::stringstream cmOutput;
  std::stringstream mmOutput;

  // start tracing
  runtime_->startDeviceTracing(stream, &mmOutput, &cmOutput);

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

  runtime_->stopDeviceTracing(stream);

  runtime_->waitForStream(stream);

  ASSERT_EQ(random_trash, result);
  RT_LOG(INFO) << "CM TRACE size: " << cmOutput.str().size();
  RT_LOG(INFO) << "MM TRACE size: " << mmOutput.str().size();
}

} // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
