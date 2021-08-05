//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "common/Constants.h"
#include "runtime/IRuntime.h"
#include "utils.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logger.h>

#include <experimental/filesystem>
#include <fstream>
#include <ios>
#include <random>

namespace {

struct DeviceErrors : public Fixture {
  DeviceErrors() {
    auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    init(std::move(deviceLayer));
    kernel_ = loadKernel("add_vector.elf");
  }
  rt::KernelId kernel_;
};
// Load and removal of a single kernel.
TEST_F(DeviceErrors, KernelLaunchInvalidMask) {
  std::array<std::byte, 64> dummyArgs;

  runtime_->kernelLaunch(defaultStream_, kernel_, dummyArgs.data(), sizeof(dummyArgs), 0UL);
  runtime_->waitForStream(defaultStream_);
  auto errors = runtime_->retrieveStreamErrors(defaultStream_);
  EXPECT_EQ(errors.size(), 1UL);
  bool callbackExecuted = false;
  runtime_->setOnStreamErrorsCallback([&callbackExecuted](auto, const auto&) { callbackExecuted = true; });
  runtime_->kernelLaunch(defaultStream_, kernel_, dummyArgs.data(), sizeof(dummyArgs), 0UL);
  runtime_->waitForStream(defaultStream_);
  EXPECT_TRUE(callbackExecuted);
}

} // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
