//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include "DeviceAPI/Commands.h"
#include "esperanto/runtime/Support/Logging.h"

using namespace et_runtime::device_api;

// Wait for device-fw to raise an interrupt from device to host
TEST_F(MailboxEmuTest, waitForHostInterrupt) {
  // Do nothing the test fixture should do the above
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  target_device->boot(0x8000001000);
  auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));
  ASSERT_TRUE(res);
}

TEST_F(MailboxEmuTest, waitForMailboxReady) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  target_device->boot(0x8000001000);
  auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));

  auto &mb_emu = target_device->mailboxDev();
  auto ready = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(ready);
}

TEST_F(MailboxEmuTest, resetMailBox) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  target_device->boot(0x8000001000);
  auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));

  auto &mb_emu = target_device->mailboxDev();
  auto success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  success = mb_emu.reset();
  EXPECT_TRUE(success);

  success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);
}

TEST_F(DeviceFWTest, reflectTest) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);


  auto reflect_cmd = std::make_shared<ReflectTestCommand>();
  dev_->defaultStream().addCommand(reflect_cmd);

  auto future = reflect_cmd->getFuture();
  auto respose = future.get();
  RTDEBUG << "Reflect message received \n";
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
