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

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
