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
#include "RuntimeFixture.h"
#include "RuntimeImp.h"
#include "common/Constants.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <mutex>
#include <random>
#include <thread>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "cannot include the filesystem library"
#endif

using namespace rt;
using namespace std::chrono_literals;
namespace {
class TestAbort : public RuntimeFixture {
public:
  void SetUp() override {
    RuntimeFixture::SetUp();
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
  if (RuntimeFixture::sDlType == RuntimeFixture::DeviceLayerImp::SYSEMU) {
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
  rimp->setSentCommandCallback(devices_[0], [this, &done](rt::Command const* cmd) {
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

TEST_F(TestAbort, abortShouldDump) {
  if (RuntimeFixture::sDlType == RuntimeFixture::DeviceLayerImp::SYSEMU) {
    RT_LOG(WARNING) << "Abort Command is not supported in sysemu. Returning.";
    FAIL();
  }

  runtime_->setOnStreamErrorsCallback(nullptr);
  RT_LOG(INFO) << "Sending kernel launch which will be aborted later";
  auto dumpFileName = "coredump-temp.dump";
  std::remove(dumpFileName);
  ASSERT_FALSE(fs::exists(dumpFileName));

  auto rimp = static_cast<rt::RuntimeImp*>(runtime_.get());
  std::once_flag flag;
  rimp->setSentCommandCallback(devices_[0], [this, &flag](rt::Command const* cmd) {
    std::call_once(flag, [this, cmd] {
      RT_LOG(INFO) << "Command sent: " << cmd << ". Now aborting stream.";
      runtime_->abortStream(defaultStreams_[0]);
    });
  });

  runtime_->kernelLaunch(defaultStreams_[0], kernelHang_, fakeArgs_.data(), fakeArgs_.size(), 0x1UL, true, false,
                         std::nullopt, dumpFileName);

  RT_LOG(INFO) << "Waiting for stream to finish.";
  while (!runtime_->waitForStream(defaultStreams_[0], 1s))
    ;
  RT_LOG(INFO) << "Stream finished.";
  ASSERT_TRUE(fs::exists(dumpFileName));
  std::remove(dumpFileName);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(devices_[0])), dev::DeviceState::Ready);
}

TEST_F(TestAbort, abortStream) {
  if (RuntimeFixture::sDlType == RuntimeFixture::DeviceLayerImp::SYSEMU) {
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
  rimp->setSentCommandCallback(devices_[0], [this, &done, &commandsSent, commandsToSend](rt::Command const*) {
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
  if (RuntimeFixture::sDlType == RuntimeFixture::DeviceLayerImp::SYSEMU) {
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
  rimp->setSentCommandCallback(devices_[0], [this, rimp](rt::Command const* cmd) {
    // Remove the callback
    rimp->setSentCommandCallback(devices_[0], std::function<void(rt::Command const*)>());

    RT_LOG(INFO) << "Command sent: " << cmd << ". Now aborting stream.";
    runtime_->abortStream(defaultStreams_[0]);
    RT_LOG(INFO) << "Waiting for stream to finish.";
    runtime_->waitForStream(defaultStreams_[0]);
  });
  RT_LOG(INFO) << "Sending kernel launch which will be aborted later";
  runtime_->kernelLaunch(defaultStreams_[0], kernelHang_, fakeArgs_.data(), fakeArgs_.size(), 0x1UL);
  for (int i = 0; i < 200; i++) {
    if (kernelAbortCalled && errorReported) {
      break;
    }
    RT_LOG(INFO) << "Not done yet, waiting. Error reported? " << errorReported
                 << " Kernel aborted error callback called? " << kernelAbortCalled;
    std::this_thread::sleep_for(10ms);
  }

  ASSERT_TRUE(errorReported);
  ASSERT_TRUE(kernelAbortCalled);
  ASSERT_EQ(deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(devices_[0])), dev::DeviceState::Ready);
}

int main(int argc, char** argv) {
  RuntimeFixture::ParseArguments(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
