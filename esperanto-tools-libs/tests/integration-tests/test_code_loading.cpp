//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeImp.h"
#include "TestUtils.h"
#include "Utils.h"
#include "common/Constants.h"
#include "device-layer/IDeviceLayer.h"
#include "runtime/IRuntime.h"
#include "sw-sysemu/SysEmuOptions.h"

#include <hostUtils/logging/Logger.h>
#include <experimental/filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ios>

namespace fs = std::experimental::filesystem;

namespace {

class TestCodeLoading : public ::testing::Test {
public:
  void SetUp() override {
    deviceLayer_ = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    runtime_ = rt::IRuntime::create(deviceLayer_.get());
    devices_ = runtime_->getDevices();
    stream_ = runtime_->createStream(devices_[0]);
    ASSERT_GE(devices_.size(), 1);
    auto elf_file = std::ifstream((fs::path(KERNELS_DIR) / fs::path("add_vector.elf")).string(), std::ios::in | std::ios::binary);
    ASSERT_TRUE(elf_file.is_open());
    elf_file.seekg(0, std::ios::end);
    auto size = elf_file.tellg();
    ASSERT_GT(size, 0);
    addVectorContent_.resize(static_cast<unsigned long>(size));
    elf_file.seekg(0);
    elf_file.read(reinterpret_cast<char*>(addVectorContent_.data()), size);
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
    runtime_->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  }

  void TearDown() override {
    runtime_->waitForStream(stream_);
    runtime_->destroyStream(stream_);
    runtime_.reset();
  }

  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  rt::RuntimePtr runtime_;
  std::vector<rt::DeviceId> devices_;
  std::vector<std::byte> addVectorContent_;
  rt::StreamId stream_;
};

// Load and removal of a single kernel.
TEST_F(TestCodeLoading, LoadKernel) {

  rt::LoadCodeResult loadCodeResult = runtime_->loadCode(stream_, addVectorContent_.data(), addVectorContent_.size());
  EXPECT_NO_THROW(runtime_->unloadCode(loadCodeResult.kernel_));
  // if we unload again the same kernel we should expect to throw an exception
  EXPECT_THROW(runtime_->unloadCode(loadCodeResult.kernel_), rt::Exception);
}

TEST_F(TestCodeLoading, MultipleLoads) {
  std::vector<rt::LoadCodeResult> loadCodeResults;

  RT_LOG(INFO) << "Loading 100 kernels";
  for (int i = 0; i < 100; ++i) {
    EXPECT_NO_THROW(
      loadCodeResults.emplace_back(runtime_->loadCode(stream_, addVectorContent_.data(), addVectorContent_.size())));
  }
  for (auto it = begin(loadCodeResults) + 1; it != end(loadCodeResults); ++it) {
    EXPECT_LT(static_cast<uint32_t>((it - 1)->kernel_), static_cast<uint32_t>(it->kernel_));
  }
  RT_LOG(INFO) << "Unloading 100 kernels";
  for (auto lcr : loadCodeResults) {
    EXPECT_NO_THROW(runtime_->unloadCode(lcr.kernel_));
  }
}

} // namespace

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
