//******************************************************************************
// Copyright (C) 2020,2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Constants.h"
#include "RuntimeImp.h"
#include "TestUtils.h"
#include "common/Constants.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <mutex>
#include <random>
#include <thread>
using namespace rt;
using namespace std::chrono_literals;
namespace {
class TestAbort : public Fixture {
public:
  void SetUp() override {
    Fixture::SetUp();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
    kernelHang_ = loadKernel("hang.elf");
    runtime_->waitForStream(defaultStreams_[0]);
  }
  std::array<std::byte, 32> fakeArgs_;
  KernelId kernelHang_;
};
} // namespace
TEST_F(TestAbort, abortCommand) {
  if (Fixture::sMode == Fixture::Mode::SYSEMU) {
    RT_LOG(WARNING) << "Abort Command is not supported in sysemu. Returning.";
    FAIL();
  }

  bool errorReported = false;

  runtime_->setOnStreamErrorsCallback([&errorReported](rt::EventId, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::KernelLaunchHostAborted);
    errorReported = true;
  });
  auto rimp = static_cast<rt::RuntimeImp*>(runtime_.get());
  bool done = false;
  rimp->setSentCommandCallback(devices_[0], [this, &done](rt::Command* cmd) {
    RT_LOG(INFO) << "Command sent: " << cmd << ". Now aborting stream.";
    runtime_->abortStream(defaultStreams_[0]);
    RT_LOG(INFO) << "Waiting for stream to finish.";
    runtime_->waitForStream(defaultStreams_[0]);
    done = true;
  });
  RT_LOG(INFO) << "Sending kernel launch which will be aborted later";
  runtime_->kernelLaunch(defaultStreams_[0], kernelHang_, fakeArgs_.data(), fakeArgs_.size(), 0x1UL);
  while (!done) {
    RT_LOG(INFO) << "Not done yet, waiting. Error reported? " << errorReported;
    std::this_thread::sleep_for(1s);
  }

  ASSERT_TRUE(errorReported);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(devices_[0])), dev::DeviceState::Ready);
}

TEST_F(TestAbort, abortStream) {
  if (Fixture::sMode == Fixture::Mode::SYSEMU) {
    RT_LOG(WARNING) << "Abort Stream is not supported in sysemu. Returning.";
    FAIL();
  }
  auto eventsReported = std::set<rt::EventId>{};
  runtime_->setOnStreamErrorsCallback([&eventsReported](rt::EventId event, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::KernelLaunchHostAborted);
    eventsReported.emplace(event);
  });

  // then lets queue several memcpy commands
  auto eventsSubmitted = std::set<rt::EventId>{};
  auto rimp = static_cast<rt::RuntimeImp*>(runtime_.get());
  bool done = false;
  auto commandsSent = 0U;
  auto commandsToSend = 10U;
  rimp->setSentCommandCallback(devices_[0], [this, &done, &commandsSent, commandsToSend](rt::Command*) {
    commandsSent++;
    if (commandsSent == commandsToSend) {
      RT_LOG(INFO) << "All commands sent. Now aborting stream.";
      runtime_->abortStream(defaultStreams_[0]);
      RT_LOG(INFO) << "Waiting for stream to finish.";
      runtime_->waitForStream(defaultStreams_[0]);
      done = true;
    };
  });
  for (auto i = 0U; i < commandsToSend; ++i) {
    eventsSubmitted.emplace(
      runtime_->kernelLaunch(defaultStreams_[0], kernelHang_, fakeArgs_.data(), fakeArgs_.size(), 0x1UL));
  }
  while (!done) {
    RT_LOG(INFO) << "Not done yet, waiting.";
    std::this_thread::sleep_for(1s);
  }
  ASSERT_EQ(eventsReported, eventsSubmitted);
}

TEST_F(TestAbort, kernelAbortedCallback) {
  if (Fixture::sMode == Fixture::Mode::SYSEMU) {
    RT_LOG(WARNING) << "Abort Command is not supported in sysemu. Returning.";
    FAIL();
  }

  bool errorReported = false;
  bool kernelAbortCalled = false;

  runtime_->setOnStreamErrorsCallback([&errorReported](rt::EventId, const rt::StreamError& error) {
    ASSERT_EQ(error.errorCode_, rt::DeviceErrorCode::KernelLaunchHostAborted);
    errorReported = true;
  });
  runtime_->setOnKernelAbortedErrorCallback([&kernelAbortCalled](auto, std::byte* addr, size_t size, auto clearFunc) {
    ASSERT_NE(addr, nullptr);
    ASSERT_EQ(size, kExceptionBufferSize);
    kernelAbortCalled = true;
    clearFunc();
  });

  auto rimp = static_cast<rt::RuntimeImp*>(runtime_.get());
  rimp->setSentCommandCallback(devices_[0], [this](rt::Command* cmd) {
    RT_LOG(INFO) << "Command sent: " << cmd << ". Now aborting stream.";
    runtime_->abortStream(defaultStreams_[0]);
    RT_LOG(INFO) << "Waiting for stream to finish.";
    runtime_->waitForStream(defaultStreams_[0]);
  });
  RT_LOG(INFO) << "Sending kernel launch which will be aborted later";
  runtime_->kernelLaunch(defaultStreams_[0], kernelHang_, fakeArgs_.data(), fakeArgs_.size(), 0x1UL);
  while (!kernelAbortCalled) {
    RT_LOG(INFO) << "Not done yet, waiting. Error reported? " << errorReported
                 << " Kernel aborted error callback called? " << kernelAbortCalled;
    std::this_thread::sleep_for(10ms);
  }

  ASSERT_TRUE(errorReported);
  ASSERT_TRUE(kernelAbortCalled);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(devices_[0])), dev::DeviceState::Ready);
}

int main(int argc, char** argv) {
  ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
