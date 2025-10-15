//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "runtime/IRuntime.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>

using namespace rt;

namespace {
class TestStackConfiguration : public RuntimeFixture {
public:
  void SetUp() override {
    RuntimeFixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    stackConfKernel_ = loadKernel("custom_stack.elf");
    runtime_->waitForStream(defaultStreams_[0]);
    auto devProps = runtime_->getDeviceProperties(devices_[0]);
    kNumHarts_ = 64 * devProps.availableShires_;
  }

  KernelId stackConfKernel_;
  size_t kNumHarts_;
};
} // namespace

TEST_F(TestStackConfiguration, goodStackConfiguration) {
  constexpr size_t stackPerHart = 8192;
  size_t stackBuffferSize = stackPerHart * kNumHarts_;
  uint64_t stackofsize = 7500;

  runtime_->setOnStreamErrorsCallback(nullptr);

  struct StackSize {
    uint64_t stack_size;
  } stackSize{stackofsize};

  KernelLaunchOptions kOpts;

  std::byte* addrptr = runtime_->mallocDevice(devices_[0], stackBuffferSize, 4096);

  kOpts.setStackConfig(addrptr, stackBuffferSize);

  runtime_->kernelLaunch(defaultStreams_[0], stackConfKernel_, reinterpret_cast<std::byte*>(&stackSize),
                         sizeof(stackSize), kOpts);
  runtime_->waitForStream(defaultStreams_[0]);
  auto errors = runtime_->retrieveStreamErrors(defaultStreams_[0]);
  ASSERT_EQ(errors.size(), 0UL);
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
