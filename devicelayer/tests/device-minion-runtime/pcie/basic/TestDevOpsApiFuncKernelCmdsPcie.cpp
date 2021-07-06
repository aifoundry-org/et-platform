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

class TestDevOpsApiFuncKernelCmdsPcie : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchAddVectorKernel_PositiveTesting_4_1) {
  launchAddVectorKernel_PositiveTesting_4_1(0x1);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchAddVectorKernel_VariableShireMasks) {
  launchAddVectorKernel_PositiveTesting_4_1(0x1);                /* Shire 0 */
  launchAddVectorKernel_PositiveTesting_4_1(0x3);                /* Shire 0, 1 */
  launchAddVectorKernel_PositiveTesting_4_1(0x3 | (1ull << 32)); /* Shire 0, 1, 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchUberKernel_PositiveTesting_4_4) {
  launchUberKernel_PositiveTesting_4_4(0x3 | (1ull << 32)); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchEmptyKernel_PositiveTesting_4_5) {
  launchEmptyKernel_PositiveTesting_4_5(0x3 | (1ull << 32)); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchExceptionKernel_NegativeTesting_4_6) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: launchExceptionKernel_NegativeTesting_4_6, not supported on loopback driver"
                 << std::endl;
    return;
  }
  launchExceptionKernel_NegativeTesting_4_6(0x3); /* Shire 0 and 1 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, abortHangKernel_PositiveTesting_5_1) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: abortHangKernel_PositiveTesting_5_1, not supported on loopback driver" << std::endl;
    return;
  }
  launchHangKernel(0x3, true); /* Shire 0 and 1 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, kernelAbortCmd_InvalidTagIdNegativeTesting_6_2) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: kernelAbortCmd_InvalidTagIdNegativeTesting_6_2, not supported on loopback driver"
                 << std::endl;
    return;
  }
  kernelAbortCmd_InvalidTagIdNegativeTesting_6_2();
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Tests                          *
*                                                         *
**********************************************************/

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchAddVectorKernelDMAList_PositiveTesting_4_10) {
  launchAddVectorKernelListCmd(0x1);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchAddVectorKernelDMAList_VariableShireMasks_4_11) {
  launchAddVectorKernelListCmd(0x1);                /* Shire 0 */
  launchAddVectorKernelListCmd(0x3);                /* Shire 0, 1 */
  launchAddVectorKernelListCmd(0x3 | (1ull << 32)); /* Shire 0, 1, 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchUberKernelDMAList_PositiveTesting_4_14) {
  launchUberKernelListCmd(0x3 | (1ull << 32)); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchEmptyKernelDMAList_PositiveTesting_4_15) {
  launchEmptyKernelListCmd(0x3 | (1ull << 32)); /* Shire 0, 1 and 32 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, launchExceptionKernelDMAList_NegativeTesting_4_16) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: launchExceptionKernelDMAList_NegativeTesting_4_16, not supported on loopback driver"
                 << std::endl;
    return;
  }
  launchExceptionKernelListCmd(0x3); /* Shire 0 and 1 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, abortHangKernelDMAList_PositiveTesting_5_11) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: abortHangKernelDMAList_PositiveTesting_5_11, not supported on loopback driver" << std::endl;
    return;
  }
  launchHangKernelListCmd(0x3, true); /* Shire 0 and 1 */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, kernelLaunchCmd_InvalidAddressNegativeTesting_6_3) {
  kernelLaunchCmd_InvalidAddressNegativeTesting(0x3);
}
// TODO: Enable this test once runtime has switched to using new kernel launch command
TEST_F(TestDevOpsApiFuncKernelCmdsPcie, DISABLED_kernelLaunchCmd_InvalidArgSizeNegativeTesting_6_4) {
  kernelLaunchCmd_InvalidArgSizeNegativeTesting(0x3);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, kernelLaunchCmd_ExceptionKernelNegativeTesting_6_5) {
  launchExceptionKernelListCmd(0x3);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, kernelLaunchCmd_AbortKernelNegativeTesting_6_7) {
  launchHangKernelListCmd(0x3, true);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcie, kernelLaunchCmd_InvalidShireMaskNegativeTesting_6_8) {
  kernelLaunchCmd_InvalidShireMaskNegativeTesting(0x0);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
