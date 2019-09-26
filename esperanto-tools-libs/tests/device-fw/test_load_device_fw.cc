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

TEST_F(DeviceFWTest, loadOnSysEMU) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
}


// FIXME currently the test boots device-fw in the future we need to
// wait until the mailbox is initialized
TEST_F(DeviceFWTest, waitForMailboxReady) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
  auto& target_device = dev_->getTargetDevice();

  // For now let the simulator free run for some time.
  // FIXME set the boot address to be the load address of the machine minion-ELF
  target_device.boot(0x8000001000, 0x8000001000);
  std::this_thread::sleep_for(std::chrono::seconds(10));
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
