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
class TestAbort : public Fixture {
public:
  void SetUp() override {
    Fixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
  }
};
} // namespace
TEST_F(TestAbort, abortCommand) {
  if (Fixture::sMode == Fixture::Mode::SYSEMU) {
    RT_LOG(WARNING) << "Abort Command is not supported in sysemu. Returning.";
    FAIL();
  }
  std::array<std::byte, 1024> hostMem;
  bool errorReported = false;
  auto dst = runtime_->mallocDevice(defaultDevice_, hostMem.size());
  runtime_->setOnStreamErrorsCallback([&errorReported](rt::EventId, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::DmaAborted);
    errorReported = true;
  });
  auto evt = runtime_->memcpyHostToDevice(defaultStream_, hostMem.data(), dst, hostMem.size());
  runtime_->abortCommand(evt);
  runtime_->waitForStream(defaultStream_);
  ASSERT_TRUE(errorReported);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(defaultDevice_)), dev::DeviceState::Ready);
}

TEST_F(TestAbort, abortStream) {
  if (Fixture::sMode == Fixture::Mode::SYSEMU) {
    RT_LOG(WARNING) << "Abort Stream is not supported in sysemu. Returning.";
    FAIL();
  }
  auto eventsReported = std::set<rt::EventId>{};
  runtime_->setOnStreamErrorsCallback([&eventsReported](rt::EventId event, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::DmaAborted);
    eventsReported.emplace(event);
  });

  std::array<std::byte, 1024> hostMem;
  // then lets queue several memcpy commands
  auto dst = runtime_->mallocDevice(defaultDevice_, hostMem.size());
  auto eventsSubmitted = std::set<rt::EventId>{};
  for (int i = 0; i < 10; ++i) {
    eventsSubmitted.emplace(runtime_->memcpyHostToDevice(defaultStream_, hostMem.data(), dst, hostMem.size()));
  }
  runtime_->abortStream(defaultStream_);
  runtime_->waitForStream(defaultStream_);
  ASSERT_EQ(eventsReported, eventsSubmitted);
}

int main(int argc, char** argv) {
  Fixture::sMode = IsPcie(argc, argv) ? Fixture::Mode::PCIE : Fixture::Mode::SYSEMU;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
