//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "Utils.h"
#include "common/Constants.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logger.h>

#include <experimental/filesystem>
#include <fstream>
#include <ios>
#include <random>

struct DeviceErrors : public RuntimeFixture {
  void SetUp() override {
    RuntimeFixture::SetUp();
    // unset the callback because we don't want to fail on purpose, thats part of these tests
    runtime_->setOnStreamErrorsCallback(nullptr);
    add_vector_kernel = loadKernel("add_vector.elf");
    exception_kernel = loadKernel("exception.elf");
  }
  rt::KernelId add_vector_kernel;
  rt::KernelId exception_kernel;
};

TEST_F(DeviceErrors, KernelLaunchInvalidMask) {
  std::array<std::byte, 64> dummyArgs;

  EXPECT_THROW(runtime_->kernelLaunch(defaultStreams_[0], add_vector_kernel, dummyArgs.data(), sizeof(dummyArgs), 0UL),
               rt::Exception);
  runtime_->waitForStream(defaultStreams_[0]);
  defaultStreams_.clear();
  runtime_.reset();
}

TEST_F(DeviceErrors, KernelLaunchException) {
  std::array<std::byte, 64> dummyArgs;

  // Launch Kernel on all 32 Shires including Sync Minions
  runtime_->kernelLaunch(defaultStreams_[0], exception_kernel, dummyArgs.data(), sizeof(dummyArgs), 0x1FFFFFFFFUL);
  runtime_->waitForStream(defaultStreams_[0]);
  auto errors = runtime_->retrieveStreamErrors(defaultStreams_[0]);
  ASSERT_EQ(errors.size(), 1UL);
  bool callbackExecuted = false;
  runtime_->setOnStreamErrorsCallback([&callbackExecuted](auto, const rt::StreamError& error) {
    callbackExecuted = true;
    RT_LOG(INFO) << "Error cm mask: " << *error.cmShireMask_;
  });
  // Launch Kernel on all 32 Shires including Sync Minions
  runtime_->kernelLaunch(defaultStreams_[0], exception_kernel, dummyArgs.data(), sizeof(dummyArgs), 0x1FFFFFFFFUL);
  runtime_->waitForStream(defaultStreams_[0]);
  runtime_.reset();
  defaultStreams_.clear();
  ASSERT_TRUE(callbackExecuted);
  RT_LOG(INFO) << "This is expected, part of the test. Stream error message: \n" << errors[0].getString();
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
