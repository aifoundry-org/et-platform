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
 * Test Labels: FC, NIGHTLY, PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiNightlyKernelCmdsPcie : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchAddVectorKernel_PositiveTesting_4_1) {
  launchAddVectorKernel_PositiveTesting_4_1(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchAddVectorKernel_VariableShireMasks) {
  launchAddVectorKernel_PositiveTesting_4_1(0x1);         /* Shire 0 */
  launchAddVectorKernel_PositiveTesting_4_1(0x3);         /* Shire 0-1 */
  launchAddVectorKernel_PositiveTesting_4_1(0x7);         /* Shire 0-2 */
  launchAddVectorKernel_PositiveTesting_4_1(0xF);         /* Shire 0-4 */
  launchAddVectorKernel_PositiveTesting_4_1(0xFF);        /* Shire 0-8 */
  launchAddVectorKernel_PositiveTesting_4_1(0xFFFF);      /* Shire 0-16 */
  launchAddVectorKernel_PositiveTesting_4_1(0x1FFFFFFFF); /* Shire 0-32 */
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchUberKernel_PositiveTesting_4_4) {
  launchUberKernel_PositiveTesting_4_4(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchExceptionKernel_NegativeTesting_4_6) {
  launchExceptionKernel_NegativeTesting_4_6(0x1FFFFFFFF); // all shires
}
/* TODO: SW-7827
TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, abortHangKernel_PositiveTesting_5_1) {
  abortHangKernel_PositiveTesting_4_10(0x1FFFFFFFF); // all shires
}
*/
TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelLaunchCmds_3_1) {
  backToBackSameKernelLaunchCmds_3_1(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelLaunchCmds_3_2) {
  backToBackDifferentKernelLaunchCmds_3_2(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelLaunch_3_3) {
  backToBackEmptyKernelLaunch_3_3(0x1FFFFFFFF, false); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelLaunchFlushL3_3_4) {
  backToBackEmptyKernelLaunch_3_3(0x1FFFFFFFF, true); // all shires
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
