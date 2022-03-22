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
#include "runtime/Types.h"

#include <deviceLayer/IDeviceLayerMock.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Property;
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
    runtime_ = std::make_unique<RuntimeImp>(deviceLayer_.get(), std::move(profilerMock), getDefaultOptions());
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

MATCHER(AGetDevicesEvent, "") {
  return arg.getClass() == profiling::Class::GetDevices;
}

// getDevices generates 1 (complete) event
TEST_F(ProfilerTests, getDevices) {
  EXPECT_CALL(*profilerMock_, record(AGetDevicesEvent())).Times(1);
  runtime_->getDevices();
}

MATCHER(AMallocDevice, "") {
  return arg.getClass() == profiling::Class::MallocDevice;
}

// mallocDevice generates 1 (complete) event
TEST_F(ProfilerTests, mallocDevice) {
  EXPECT_CALL(*profilerMock_, record(AMallocDevice())).Times(1);
  runtime_->mallocDevice(DeviceId(0), 10u);
}

MATCHER(AFreeDevice, "") {
  return arg.getClass() == profiling::Class::FreeDevice;
}

// freeDevice generates 1 (complete) event
TEST_F(ProfilerTests, freeDevice) {
  auto devId = DeviceId(0);
  auto d_ptr = runtime_->mallocDevice(devId, 10u);

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AFreeDevice())).Times(1);
  runtime_->freeDevice(devId, d_ptr);
  // ------- ^^^^^^^^^^^^^^^ -------
}

MATCHER(ACreateStream, "") {
  return arg.getClass() == profiling::Class::CreateStream;
}

// createStream generates 1 (complete) event
TEST_F(ProfilerTests, createStream) {
  EXPECT_CALL(*profilerMock_, record(ACreateStream())).Times(1);
  runtime_->createStream(DeviceId(0));
}

MATCHER(ADestroyStream, "") {
  return arg.getClass() == profiling::Class::DestroyStream;
}

// destroyStream generates 1 (complete) event
TEST_F(ProfilerTests, destroyStream) {
  auto stream = runtime_->createStream(DeviceId(0));

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(ADestroyStream())).Times(1);
  runtime_->destroyStream(stream);
  // ------- ^^^^^^^^^^^^^^^ -------
}

MATCHER(AWaitForStream, "") {
  return arg.getClass() == profiling::Class::WaitForStream;
}

// waitForStream generates 1 (complete) event
TEST_F(ProfilerTests, waitForStream) {
  auto stream = runtime_->createStream(DeviceId(0));

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AWaitForStream())).Times(1);
  runtime_->waitForStream(stream);
  // ------- ^^^^^^^^^^^^^^^ -------
}

MATCHER(AWaitForEvent, "") {
  return arg.getClass() == profiling::Class::WaitForEvent;
}

// waitForEvent generates 1 (complete) event
TEST_F(ProfilerTests, waitForEvent) {
  auto stream = runtime_->createStream(DeviceId(0));
  auto event_id = runtime_->abortStream(stream);

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AWaitForEvent())).Times(1);
  runtime_->waitForEvent(event_id);
  // ------- ^^^^^^^^^^^^^^^ -------
}

MATCHER(AMemcpyHostToDevice, "") {
  return arg.getClass() == profiling::Class::MemcpyHostToDevice;
}
MATCHER(ACommandSent, "") {
  return arg.getClass() == profiling::Class::CommandSent;
}
MATCHER(AResponseReceived, "") {
  return arg.getClass() == profiling::Class::ResponseReceived;
}
MATCHER(ADispatchEventReceived, "") {
  return arg.getClass() == profiling::Class::DispatchEvent;
}

// memcpy H2D only generates 1 (complete) event
TEST_F(ProfilerTests, memcpyHostToDevice) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto d_ptr = runtime_->mallocDevice(devId, 10u);
  std::array<std::byte, 10u> h_buffer;

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AMemcpyHostToDevice())).Times(1);
  EXPECT_CALL(*profilerMock_, record(ACommandSent())).Times(AtLeast(1));
  EXPECT_CALL(*profilerMock_, record(AResponseReceived())).Times(AtLeast(1)); // same
  EXPECT_CALL(*profilerMock_, record(ADispatchEventReceived())).Times(2);
  runtime_->memcpyHostToDevice(stream, h_buffer.data(), d_ptr, h_buffer.size());
  // ------- code under test -------

  sleep(1); // give time to the helper threads to record CommandSent/ResponseReceived evts
  EXPECT_CALL(*profilerMock_, record(AWaitForStream())).Times(1);
  runtime_->waitForStream(stream);
}

MATCHER(ALoadCode, "") {
  return arg.getClass() == profiling::Class::LoadCode;
}

// Obs: RuntimeImpl::loadCode relies on code (loading elf)
// that cannot be mocked.
// This test then loads a real elf
TEST_F(ProfilerTests, loadCode) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + "add_vector.elf");
  EXPECT_FALSE(kernelContent.empty());

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(ALoadCode())).Times(1);
  EXPECT_CALL(*profilerMock_, record(ACommandSent())).Times(AtLeast(1));
  EXPECT_CALL(*profilerMock_, record(AResponseReceived())).Times(AtLeast(1)); // same
  EXPECT_CALL(*profilerMock_, record(ADispatchEventReceived())).Times(2);
  runtime_->loadCode(stream, kernelContent.data(), kernelContent.size());
  // ------- code under test -------

  sleep(1); // give time to the helper threads to record CommandSent/ResponseReceived evts
  EXPECT_CALL(*profilerMock_, record(AWaitForStream())).Times(1);
  runtime_->waitForStream(stream);
}

MATCHER(AUnloadCode, "") {
  return arg.getClass() == profiling::Class::UnloadCode;
}

TEST_F(ProfilerTests, unloadCode) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + "empty.elf");
  EXPECT_FALSE(kernelContent.empty());
  auto res = runtime_->loadCode(stream, kernelContent.data(), kernelContent.size());
  auto kernel = res.kernel_;
  runtime_->waitForStream(stream);

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AUnloadCode())).Times(1);
  EXPECT_CALL(*profilerMock_, record(ACommandSent())).Times(AtLeast(0));
  EXPECT_CALL(*profilerMock_, record(AResponseReceived())).Times(AtLeast(0)); // same
  runtime_->unloadCode(kernel);
  // ------- code under test -------

  sleep(1); // give time to the helper threads to record CommandSent/ResponseReceived evts
  EXPECT_CALL(*profilerMock_, record(AWaitForStream())).Times(1);
  runtime_->waitForStream(stream);
}

MATCHER(AKernelLaunch, "") {
  return arg.getClass() == profiling::Class::KernelLaunch;
}

TEST_F(ProfilerTests, kernelLaunch) {
  auto devId = DeviceId(0);
  auto stream = runtime_->createStream(devId);
  auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + "empty.elf");
  EXPECT_FALSE(kernelContent.empty());
  auto res = runtime_->loadCode(stream, kernelContent.data(), kernelContent.size());
  auto kernel = res.kernel_;
  runtime_->waitForStream(stream);

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(AKernelLaunch())).Times(1);
  EXPECT_CALL(*profilerMock_, record(ACommandSent())).Times(AtLeast(1));
  EXPECT_CALL(*profilerMock_, record(AResponseReceived())).Times(AtLeast(1)); // same
  EXPECT_CALL(*profilerMock_, record(ADispatchEventReceived())).Times(1);
  runtime_->kernelLaunch(stream, kernel, nullptr, 0, 0x1);
  // ------- code under test -------

  sleep(1); // give time to the helper threads to record CommandSent/ResponseReceived evts
  EXPECT_CALL(*profilerMock_, record(AWaitForStream())).Times(1);
  runtime_->waitForStream(stream);
}

TEST_F(ProfilerTests, DISABLED_CommandSender) {
  std::vector<std::byte> commandData(64);

  auto header = reinterpret_cast<device_ops_api::cmn_header_t*>(commandData.data());
  // dummy msg_id to make it work on deviceLayerFake
  header->msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  header->tag_id = device_ops_api::tag_id_t(0);

  CommandSender cs(*deviceLayer_, *profilerMock_, 0, 0);

  // ------- code under test -------
  EXPECT_CALL(*profilerMock_, record(_)).Times(1);
  cs.send(Command{commandData, cs, EventId(0), false, true});
  // ------- code under test -------
}

} // end namespace rt

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
