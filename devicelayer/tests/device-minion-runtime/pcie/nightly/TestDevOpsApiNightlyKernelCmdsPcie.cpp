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

using namespace dev::dl_tests;

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

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceSingleQueue_3_1) {
  backToBackSameKernelLaunchCmds_3_1(true, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelLaunchCmdsSingleDeviceMultiQueue_3_2) {
  backToBackSameKernelLaunchCmds_3_1(true, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelLaunchCmdsMultiDeviceSingleQueue_3_3) {
  backToBackSameKernelLaunchCmds_3_1(false, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelLaunchCmdsMultiDeviceMultiQueue_3_4) {
  backToBackSameKernelLaunchCmds_3_1(false, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_5) {
  backToBackDifferentKernelLaunchCmds_3_2(true, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsSingleDeviceMultileQueue_3_4) {
  backToBackDifferentKernelLaunchCmds_3_2(true, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsMultiDeviceSingleQueue_3_7) {
  backToBackDifferentKernelLaunchCmds_3_2(false, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelLaunchCmdsMultiDeviceMultileQueue_3_8) {
  backToBackDifferentKernelLaunchCmds_3_2(false, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelLaunch_3_9) {
  backToBackEmptyKernelLaunch_3_3(100, 0x1FFFFFFFF, false); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelLaunchFlushL3_3_10) {
  backToBackEmptyKernelLaunch_3_3(100, 0x1FFFFFFFF, true); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, abortHangKernel_PositiveTesting_5_1) {
  launchHangKernel(0x1FFFFFFFF, true); // all shires
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Tests                          *
*                                                         *
**********************************************************/

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchAddVectorKernelDMAList_PositiveTesting_4_11) {
  launchAddVectorKernelListCmd(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchAddVectorKernelDMAList_VariableShireMasks) {
  launchAddVectorKernelListCmd(0x1);         /* Shire 0 */
  launchAddVectorKernelListCmd(0x3);         /* Shire 0-1 */
  launchAddVectorKernelListCmd(0x7);         /* Shire 0-2 */
  launchAddVectorKernelListCmd(0xF);         /* Shire 0-4 */
  launchAddVectorKernelListCmd(0xFF);        /* Shire 0-8 */
  launchAddVectorKernelListCmd(0xFFFF);      /* Shire 0-16 */
  launchAddVectorKernelListCmd(0x1FFFFFFFF); /* Shire 0-32 */
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchUberKernelDMAList_PositiveTesting_4_14) {
  launchUberKernelListCmd(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, launchExceptionKernelDMAList_NegativeTesting_4_16) {
  launchExceptionKernelListCmd(0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_11) {
  backToBackSameKernelLaunchListCmds(true, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsSingleDeviceMultiQueue_3_12) {
  backToBackSameKernelLaunchListCmds(true, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_13) {
  backToBackSameKernelLaunchListCmds(false, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackSameKernelDMAListLaunchCmdsMultiDeviceMultiQueue_3_14) {
  backToBackSameKernelLaunchListCmds(false, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_15) {
  backToBackDifferentKernelLaunchListCmds(true, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceMultileQueue_3_14) {
  backToBackDifferentKernelLaunchListCmds(true, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_17) {
  backToBackDifferentKernelLaunchListCmds(false, true, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceMultileQueue_3_18) {
  backToBackDifferentKernelLaunchListCmds(false, false, 100, 0x1FFFFFFFF); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelDMAListLaunch_3_19) {
  backToBackEmptyKernelLaunchListCmds(100, 0x1FFFFFFFF, false); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, backToBackEmptyKernelDMAListLaunchFlushL3_3_20) {
  backToBackEmptyKernelLaunchListCmds(100, 0x1FFFFFFFF, true); // all shires
}

TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, abortHangKernelDMAList_PositiveTesting_5_11) {
  launchHangKernelListCmd(0x1FFFFFFFF, true); // all shires
}

// this test takes too much time to execute, hence disabling it for now.
TEST_F(TestDevOpsApiNightlyKernelCmdsPcie, DISABLED_kernelLaunchCmd_HangKernelNegativeTesting_5_12) {
  launchHangKernelListCmd(0x1FFFFFFFF, false); // all shires
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
