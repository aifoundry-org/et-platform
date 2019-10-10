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


// Wait for device-fw to raise an interrupt from device to host
TEST_F(DeviceFWTest, waitForHostInterrupt) {
  // Do nothing the test fixture should do the above
}

// Wait for the mailbox to get ready
TEST_F(DeviceFWTest, waitForMailboxReady) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);
  auto &mb_emu = target_device->mailboxDev();

  auto ready = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(ready);
}

// Reset the mailbox and wait to get ready again
TEST_F(DeviceFWTest, resetMailBox) {

  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);
  auto &mb_emu = target_device->mailboxDev();

  auto success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  success = mb_emu.reset();
  EXPECT_TRUE(success);

  success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
