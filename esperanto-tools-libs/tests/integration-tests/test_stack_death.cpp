//******************************************************************************
// Copyright (C) 2023 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "runtime/IRuntime.h"
#include "gtest/gtest-death-test.h"
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

using TestStackConfigurationDeathTest = TestStackConfiguration;

TEST_F(TestStackConfigurationDeathTest, badStackConfiguration) {
  constexpr size_t stackPerHart = 4096;
  size_t stackBuffferSize = stackPerHart * kNumHarts_;
  uint64_t stackofsize = 7500;

  runtime_->setOnStreamErrorsCallback(nullptr);

  struct StackSize {
    uint64_t stack_size;
  } stackSize{stackofsize};

  KernelLaunchOptions kOpts;

  std::byte* addrptr = runtime_->mallocDevice(devices_[0], stackBuffferSize, 4096);

  kOpts.setStackConfig(addrptr, stackBuffferSize);

  ASSERT_DEATH(
    {
      runtime_->kernelLaunch(defaultStreams_[0], stackConfKernel_, reinterpret_cast<std::byte*>(&stackSize),
                             sizeof(stackSize), kOpts);
      runtime_->waitForStream(defaultStreams_[0]);
    },
    "");
}

TEST_F(TestStackConfigurationDeathTest, defaultKOptFailsOverflowStack) {
  uint64_t stackofsize = 7500;

  struct StackSize {
    uint64_t stack_size;
  } stackSize{stackofsize};

  runtime_->setOnStreamErrorsCallback(nullptr);

  KernelLaunchOptions kOpts;

  ASSERT_DEATH(
    {
      runtime_->kernelLaunch(defaultStreams_[0], stackConfKernel_, reinterpret_cast<std::byte*>(&stackSize),
                             sizeof(stackSize), kOpts);
      runtime_->waitForStream(defaultStreams_[0]);
    },
    "");
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  testing::GTEST_FLAG(death_test_style) = "threadsafe";
  return RUN_ALL_TESTS();
}