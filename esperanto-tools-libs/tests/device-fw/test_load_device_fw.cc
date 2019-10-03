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

#include <thread>
#include <chrono>

// FIXME SW-987
TEST_F(DeviceFWTest, DISABLED_loadOnSysEMU) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
}

// Wait for device-fw to raise an interrupt from device to host
// FIXME SW-987
TEST_F(DeviceFWTest, DISABLED_waitForHostInterrupt) {
  auto *target_device_ptr = &dev_->getTargetDevice();

  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  target_device->boot(0x8000001000, 0x8000001000);

  auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));
  EXPECT_TRUE(res);
}

// Wait for the mailbox to get ready
// FIXME SW-987
TEST_F(DeviceFWTest, DISABLED_waitForMailboxReady) {

  auto *target_device_ptr = &dev_->getTargetDevice();

  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  target_device->boot(0x8000001000, 0x8000001000);
  auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));
  EXPECT_TRUE(res);

  auto &mb_emu = target_device->mailboxDev();

  /// FIXME wait for device to get ready
  bool ready = false;
  for (int i = 0; i < 10 && !ready; i++) {
    ready = mb_emu.ready();
    cerr << "MB NOT READY YET \n";
    target_device->raiseDeviceInterrupt();
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  // FIXME ignore it for now
  // EXPECT_TRUE(ready);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
