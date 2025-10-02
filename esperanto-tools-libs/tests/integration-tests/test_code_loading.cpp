//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "RuntimeImp.h"
#include "Utils.h"
#include "common/Constants.h"
#include "device-layer/IDeviceLayer.h"
#include "runtime/IRuntime.h"
#include "sw-sysemu/SysEmuOptions.h"

#include <experimental/filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <ios>

namespace fs = std::experimental::filesystem;

namespace {

struct TestCodeLoading : public RuntimeFixture {
  void SetUp() override {
    RuntimeFixture::SetUp();
    // unset the callback because we don't want to fail on purpose, thats part of these tests
    runtime_->setOnStreamErrorsCallback(nullptr);
  }
};

// Load and removal of a single kernel.
TEST_F(TestCodeLoading, LoadKernel) {
  auto kernel = loadKernel("add_vector.elf");
  runtime_->waitForStream(defaultStreams_[0]);
  runtime_->unloadCode(kernel);
  // if we unload again the same kernel we should expect to throw an exception
  EXPECT_THROW(runtime_->unloadCode(kernel), rt::Exception);
}

// Test loading a kernel with bss
// flaky test don't execute wait [SW-19304]
TEST_F(TestCodeLoading, KernelWithBss) {
  auto kernel = loadKernel("bss.elf");
  auto numElems = 150U;
  auto hSrc1 = std::vector<int>(numElems);
  auto hSrc2 = std::vector<int>(numElems);
  auto hDst = std::vector<int>(numElems);
  auto dSrc1 = runtime_->mallocDevice(devices_[0], numElems * sizeof(int));
  auto dSrc2 = runtime_->mallocDevice(devices_[0], numElems * sizeof(int));
  auto dDst = runtime_->mallocDevice(devices_[0], numElems * sizeof(int));
  randomize(hSrc1, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
  randomize(hSrc2, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());

  struct {
    void* src1;
    void* src2;
    void* dst;
    int elements;
  } params{dSrc1, dSrc2, dDst, static_cast<int>(numElems)};
  runtime_->memcpyHostToDevice(defaultStreams_[0], reinterpret_cast<std::byte*>(hSrc1.data()), dSrc1,
                               numElems * sizeof(int));
  runtime_->memcpyHostToDevice(defaultStreams_[0], reinterpret_cast<std::byte*>(hSrc2.data()), dSrc2,
                               numElems * sizeof(int));
  runtime_->kernelLaunch(defaultStreams_[0], kernel, reinterpret_cast<std::byte*>(&params), sizeof(params), 0x1);
  runtime_->memcpyDeviceToHost(defaultStreams_[0], dDst, reinterpret_cast<std::byte*>(hDst.data()),
                               numElems * sizeof(int));
  runtime_->waitForStream(defaultStreams_[0]);
  runtime_->unloadCode(kernel);
  runtime_->freeDevice(devices_[0], dSrc1);
  runtime_->freeDevice(devices_[0], dSrc2);
  runtime_->freeDevice(devices_[0], dDst);
  for (auto i = 0U; i < numElems; ++i) {
    ASSERT_EQ(hDst[i], hSrc1[i] + hSrc2[i] + (i == 123 ? 1 : 0));
  }
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

  runtime_->waitForStream(defaultStreams_[0]);
  RT_LOG(INFO) << "Unloading 100 kernels";
  for (auto lcr : kernelIds) {
    runtime_->unloadCode(lcr);
  }
}

} // namespace

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
