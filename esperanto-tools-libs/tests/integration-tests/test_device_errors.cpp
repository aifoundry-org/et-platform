//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "Utils.h"
#include "common/Constants.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logger.h>

#if __has_include(<filesystem>)
#include <filesystem>
#include <bits/fs_fwd.h>
#include <bits/fs_ops.h>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#include <experimental/bits/fs_fwd.h>
#include <experimental/bits/fs_ops.h>
namespace fs = std::experimental::filesystem;
#else
#error "cannot include the filesystem library"
#endif

#include <fstream>
#include <ios>
#include <optional>
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

TEST_F(DeviceErrors, coreDump) {
  std::array<std::byte, 64> dummyArgs;
  bool callbackExecuted = false;
  runtime_->setOnStreamErrorsCallback([&callbackExecuted](auto, const rt::StreamError& error) {
    callbackExecuted = true;
    RT_LOG(INFO) << "Error cm mask: " << *error.cmShireMask_;
  });
  auto coreDumpPath = fs::current_path() / "core_dump";
  fs::remove(coreDumpPath);
  // Launch Kernel on all 32 Shires including Sync Minions
  runtime_->kernelLaunch(defaultStreams_[0], exception_kernel, dummyArgs.data(), sizeof(dummyArgs), 0x1FFFFFFFFUL, true,
                         false, std::nullopt, coreDumpPath);
  runtime_->waitForStream(defaultStreams_[0]);
  runtime_.reset();
  defaultStreams_.clear();
  ASSERT_TRUE(callbackExecuted);
  ASSERT_TRUE(fs::exists(coreDumpPath));
  ASSERT_GT(fs::file_size(coreDumpPath), 0UL);
  RT_LOG(INFO) << "CoreDump at path: " << coreDumpPath;
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
