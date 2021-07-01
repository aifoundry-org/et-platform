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

using namespace dev::dl_tests;

class TestDevOpsApiStressKernelCmdsPcie : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceSingleQueue_3_1) {
  backToBackSameKernelLaunchCmds_3_1(true, true, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceMultiQueue_3_2) {
  backToBackSameKernelLaunchCmds_3_1(true, false, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_5) {
  backToBackDifferentKernelLaunchCmds_3_2(true, true, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_6) {
  backToBackDifferentKernelLaunchCmds_3_2(true, false, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelLaunch_3_9) {
  backToBackEmptyKernelLaunch_3_3(100, 0x3 | (1ull << 32), false); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelLaunchFlushL3_3_10) {
  backToBackEmptyKernelLaunch_3_3(100, 0x3 | (1ull << 32), true); /* Shire 0, 1 and 32 */
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Tests                          *
*                                                         *
**********************************************************/


TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_11) {
  backToBackSameKernelLaunchListCmds(true, true, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsSingleDeviceMultiQueue_3_12) {
  backToBackSameKernelLaunchListCmds(true, false, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_13) {
  backToBackSameKernelLaunchListCmds(false, true, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsMultiDeviceMultiQueue_3_14) {
  backToBackSameKernelLaunchListCmds(false, false, 100, 0x1);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_15) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_3, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchListCmds(true, true, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceMultileQueue_3_16) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_4, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchListCmds(true, false, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_17) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_3, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchListCmds(false, true, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceMultileQueue_3_18) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0)
      << "Skipping: backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_4, not supported on loopback driver"
      << std::endl;
    return;
  }
  backToBackDifferentKernelLaunchListCmds(false, false, 100, 0x3);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelDMAListLaunch_3_19) {
  backToBackEmptyKernelLaunchListCmds(100, 0x3 | (1ull << 32), false); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiStressKernelCmdsPcie, backToBackEmptyKernelDMAListLaunchFlushL3_3_20) {
  backToBackEmptyKernelLaunchListCmds(100, 0x3 | (1ull << 32), true); /* Shire 0, 1 and 32 */
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
