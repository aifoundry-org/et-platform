//******************************************************************************
// Copyright (C) 2020,2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeImp.h"
#include "TestUtils.h"
#include "common/Constants.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <random>

namespace {
class TestDmaErrors : public Fixture {
public:
  void SetUp() override {
    Fixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
  }
};
} // namespace
TEST_F(TestDmaErrors, DmaOob) {
  auto dramAddr = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  auto ramUpperLimit = dramAddr + dramSize;
  std::array<std::byte, 1024> hostMem;
  bool errorReported = false;
  runtime_->setOnStreamErrorsCallback([&errorReported](rt::EventId, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::DmaInvalidAddress);
    errorReported = true;
  });
  // let's produce a OOB access
  runtime_->memcpyHostToDevice(defaultStream_, hostMem.data(), reinterpret_cast<std::byte*>(ramUpperLimit) - 512,
                               hostMem.size());
  runtime_->waitForStream(defaultStream_);
  ASSERT_TRUE(errorReported);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(defaultDevice_)), dev::DeviceState::Ready);
}

TEST_F(TestDmaErrors, DmaOobPlusCommands) {
  auto dramAddr = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  auto ramUpperLimit = dramAddr + dramSize;
  std::array<std::byte, 1024> hostMem;
  bool errorReported = false;
  runtime_->setOnStreamErrorsCallback([&errorReported](rt::EventId, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::DmaInvalidAddress);
    errorReported = true;
  });
  // let's produce a OOB access
  auto evt = runtime_->memcpyHostToDevice(defaultStream_, hostMem.data(),
                                          reinterpret_cast<std::byte*>(ramUpperLimit) - 512, hostMem.size());
  // then lets queue several memcpy commands
  auto dst = runtime_->mallocDevice(defaultDevice_, hostMem.size());
  for (int i = 0; i < 10; ++i) {
    runtime_->memcpyHostToDevice(defaultStream_, hostMem.data(), dst, hostMem.size());
  }
  runtime_->waitForEvent(evt);
  ASSERT_TRUE(errorReported);
  runtime_->setOnStreamErrorsCallback(nullptr);
  if (Fixture::sMode == Fixture::Mode::PCIE) {
    // this part of the test can only be run in PCIE because sysemu always returns "ready" in DeviceState
    // reinstantiate the runtime and check the device is ready
    runtime_ = rt::IRuntime::create(deviceLayer_.get());
    defaultStream_ = runtime_->createStream(defaultDevice_);
    ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(defaultDevice_)), dev::DeviceState::Ready);
  }
  // before ending we wait for all events to be finished
  runtime_->waitForStream(defaultStream_);
}

int main(int argc, char** argv) {
  Fixture::sMode = IsPcie(argc, argv) ? Fixture::Mode::PCIE : Fixture::Mode::SYSEMU;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
