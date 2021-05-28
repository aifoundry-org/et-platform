//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiKernelCmds.h"

/*
 * Test Labels: PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiStressKernelCmdsPcie : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceSingleQueue_3_1) {
  backToBackSameKernelLaunchCmds_3_1(true, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceMultiQueue_3_2) {
  backToBackSameKernelLaunchCmds_3_1(false, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_3) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_3, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchCmds_3_2(true, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_4) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_4, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchCmds_3_2(false, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelLaunch_3_5) {
  backToBackEmptyKernelLaunch_3_3(0x3 | (1ull << 32), false); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelLaunchFlushL3_3_6) {
  backToBackEmptyKernelLaunch_3_3(0x3 | (1ull << 32), true); /* Shire 0, 1 and 32 */
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
