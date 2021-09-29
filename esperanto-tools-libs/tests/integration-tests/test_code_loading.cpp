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

struct TestCodeLoading : public Fixture {
  TestCodeLoading() {
    auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    init(std::move(deviceLayer));
    // unset the callback because we don't want to fail on purpose, thats part of these tests
    runtime_->setOnStreamErrorsCallback(nullptr);
  }
};

// Load and removal of a single kernel.
TEST_F(TestCodeLoading, LoadKernel) {

  auto kernel = loadKernel("add_vector.elf");
  runtime_->waitForStream(defaultStream_);
  runtime_->unloadCode(kernel);
  // if we unload again the same kernel we should expect to throw an exception
  EXPECT_THROW(runtime_->unloadCode(kernel), rt::Exception);
}

TEST_F(TestCodeLoading, MultipleLoads) {
  std::vector<rt::KernelId> kernelIds;

  RT_LOG(INFO) << "Loading 100 kernels";
  for (int i = 0; i < 100; ++i) {
    kernelIds.emplace_back(loadKernel("add_vector.elf"));
  }
  for (auto it = begin(kernelIds) + 1; it != end(kernelIds); ++it) {
    EXPECT_LT(static_cast<uint32_t>(*(it - 1)), static_cast<uint32_t>(*it));
  }

  runtime_->waitForStream(defaultStream_);
  RT_LOG(INFO) << "Unloading 100 kernels";
  for (auto lcr : kernelIds) {
    runtime_->unloadCode(lcr);
  }
}

} // namespace

int main(int argc, char** argv) {
  Fixture::sPcieMode = IsPcie(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
