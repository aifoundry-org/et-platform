/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "TestUtils.h"
#include "mocks/ProfilerMock.h"
#include "gtest/gtest.h"

#include "RuntimeImp.h"

#include <deviceLayer/IDeviceLayerMock.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

namespace rt {

class ProfilerTests : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a 'Nice' DeviceLayerMock (it wont report uninsteresting calls)
    deviceLayer_ = std::make_unique<NiceMock<dev::IDeviceLayerMock>>();
    // delegate mock implementation to IDeviceLayerFake (better implementation, closer to reality than default mock)
    deviceLayer_->DelegateToFake();
    auto profilerMock = std::make_unique<NiceMock<profiling::ProfilerMock>>();
    ON_CALL(*profilerMock, record).WillByDefault([](const profiling::ProfileEvent& evt) {
      RT_LOG(INFO) << "Recording evt: " << profiling::getString(evt.getClass()) << " - "
                   << profiling::getString(evt.getType());
    });
    profilerMock_ = profilerMock.get();
    runtime_ = std::make_unique<RuntimeImp>(deviceLayer_.get(), std::move(profilerMock));
  }
  void TearDown() override {
    // Verify ProfilerMock expectations before runtime is destroyed!
    // this will 'ignore' uninteresting calls to 'record'
    Mock::VerifyAndClearExpectations(profilerMock_);

    runtime_.reset();
  }

  std::unique_ptr<dev::IDeviceLayerMock> deviceLayer_;
  std::unique_ptr<IRuntime> runtime_;
  profiling::ProfilerMock* profilerMock_;
};

TEST_F(ProfilerTests, stop_recording) {
  // when we call 'stop', we really reach the 'stop' method in the profiler mock
  EXPECT_CALL(*profilerMock_, stop).Times(1);

  auto profiler = runtime_->getProfiler();
  profiler->stop();
}

//
// Test that IRuntime public interface generates profiling events
//
TEST_F(ProfilerTests, getDevices) {
  // getDevices generates 2 events (start-stop)
  EXPECT_CALL(*profilerMock_, record).Times(2);

  runtime_->getDevices();
}

TEST_F(ProfilerTests, mallocDevice) {
  // mallocDevice generates 2 events (start-stop)
  EXPECT_CALL(*profilerMock_, record).Times(2);

  runtime_->mallocDevice(DeviceId(0), 10u);
}

TEST_F(ProfilerTests, freeDevice) {
  auto devId = DeviceId(0);
  auto d_ptr = runtime_->mallocDevice(devId, 10u);

  // freeDevice generates 2 events (start-stop)
  EXPECT_CALL(*profilerMock_, record).Times(2);

  runtime_->freeDevice(devId, d_ptr);
}

TEST_F(ProfilerTests, createStream) {
  auto devId = DeviceId(0);

  // createStream generates 2 events (start-stop)
  EXPECT_CALL(*profilerMock_, record).Times(2);

  runtime_->createStream(devId);
}

TEST_F(ProfilerTests, destroyStream) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);

  // destroyStream generates 2 events (start-stop)
  EXPECT_CALL(*profilerMock_, record).Times(2);

  runtime_->destroyStream(stream);
}

// RuntimeImpl::loadCode relies on code (loading elf)
// that cannot be mocked.
// This test then loads a real elf
TEST_F(ProfilerTests, loadCode) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + "add_vector.elf");
  EXPECT_FALSE(kernelContent.empty());

  // loadCode test generates:
  // +2 loadCode itself
  //   +2 1-mallocDevice for loading the code
  //   +2 1-mallocDevice x segment (only 1 segment)
  //   +2 1-waitForEvent x event (only 1 event)
  // +2 mandatory waitForStream
  //   +4 2-waitForEvent (2 events)
  EXPECT_CALL(*profilerMock_, record).Times(AtLeast(4)); // <-- unreliable. Improve exact count after SW-10380

  runtime_->loadCode(stream, kernelContent.data(), kernelContent.size());
  runtime_->waitForStream(stream);
}

TEST_F(ProfilerTests, kernelLaunch) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + "empty.elf");
  EXPECT_FALSE(kernelContent.empty());
  auto res = runtime_->loadCode(stream, kernelContent.data(), kernelContent.size());
  auto kernel = res.kernel_;

  // kernelLaunch test generates:
  // +2 kernelLaunch itself
  //   +2 1-waitForEvent x event (only 1 event)
  //   +1 CommandSent
  //   +1 ResponseReceived
  // +2 mandatory waitForStream
  //   +1 CommandSent
  //   +1 ResponseReceived
  EXPECT_CALL(*profilerMock_, record).Times(AtLeast(4)); //<<-- unreliable. Improve exact count after SW-10380

  runtime_->kernelLaunch(stream, kernel, nullptr, 0, 0x1);
  runtime_->waitForStream(stream);
}

} // end namespace rt

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
