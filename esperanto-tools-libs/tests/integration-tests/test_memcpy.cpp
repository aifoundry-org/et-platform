//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"

#include <fstream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ios>

#include <esperanto/runtime/Core/CommandLineOptions.h>

namespace {

class TestMemcpy : public ::testing::Test {
public:
  void SetUp() override {
    runtime_ = rt::IRuntime::create(rt::IRuntime::Kind::SysEmu);
    devices_ = runtime_->getDevices();
    ASSERT_GE(devices_.size(), 1);
  }

  void TearDown() override {
    runtime_.reset();
  }

  rt::RuntimePtr runtime_;
  std::vector<rt::DeviceId> devices_;
};

// Load and removal of a single kernel.
TEST_F(TestMemcpy, SimpleMemcpy) {

  int numElems = 1024 * 1024 * 10;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = std::vector<int>();

  for (int i = 0; i < numElems; ++i) {
    random_trash.emplace_back(rand());
  }

  // alloc memory in device
  auto sizeBytes = random_trash.size() * sizeof(int);
  auto d_buffer = runtime_->mallocDevice(dev, sizeBytes);

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(random_trash.data()), d_buffer, sizeBytes);
  auto result = std::vector<int>(numElems);
  ASSERT_NE(random_trash, result);

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(result.data()), sizeBytes);
  runtime_->waitForStream(stream);

  ASSERT_EQ(random_trash, result);
}

} // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_memcpy.cpp"});
  return RUN_ALL_TESTS();
}
