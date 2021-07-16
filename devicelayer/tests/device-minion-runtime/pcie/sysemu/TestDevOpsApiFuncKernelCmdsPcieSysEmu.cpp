//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiKernelCmds.h"

/*
 * Test Labels: PCIE-SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;

namespace {
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

class TestDevOpsApiFuncKernelCmdsPcieSysEmu : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernel_PositiveTesting_4_1) {
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernel_VariableShireMasks) {
  launchAddVectorKernel_PositiveTesting_4_1((0x1 & kSysEmuMinionShiresMask));         /* Shire 0 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x3 & kSysEmuMinionShiresMask));         /* Shire 0-1 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x7 & kSysEmuMinionShiresMask));         /* Shire 0-2 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xF & kSysEmuMinionShiresMask));         /* Shire 0-4 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xFF & kSysEmuMinionShiresMask));        /* Shire 0-8 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xFFFF & kSysEmuMinionShiresMask));      /* Shire 0-16 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x1FFFFFFFF & kSysEmuMinionShiresMask)); /* Shire 0-32 (if available) */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchUberKernel_PositiveTesting_4_4) {
  launchUberKernel_PositiveTesting_4_4(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchEmptyKernel_PositiveTesting_4_5) {
  launchEmptyKernel_PositiveTesting_4_5(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchExceptionKernel_NegativeTesting_4_6) {
  launchExceptionKernel_NegativeTesting_4_6(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, abortHangKernel_PositiveTesting_4_10) {
  launchHangKernel(kSysEmuMinionShiresMask, true);
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, allTestsConsecutively) {
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask);
  launchUberKernel_PositiveTesting_4_4(kSysEmuMinionShiresMask);
  launchEmptyKernel_PositiveTesting_4_5(kSysEmuMinionShiresMask);
  launchExceptionKernel_NegativeTesting_4_6(kSysEmuMinionShiresMask);
  launchHangKernel(kSysEmuMinionShiresMask, true);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, kernelAbortCmd_InvalidTagIdNegativeTesting_6_2) {
  kernelAbortCmd_InvalidTagIdNegativeTesting_6_2();
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernel_CM_UModeTesting_6_1) {
  // provide the new kernel elf that uses the cm-umode lib
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask, "cm_umode_test.elf");
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Tests                          *
*                                                         *
**********************************************************/

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernelDMAList_PositiveTesting_4_11) {
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernelDMAList_VariableShireMasks) {
  launchAddVectorKernelListCmd((0x1 & kSysEmuMinionShiresMask));         /* Shire 0 (if available) */
  launchAddVectorKernelListCmd((0x3 & kSysEmuMinionShiresMask));         /* Shire 0-1 (if available) */
  launchAddVectorKernelListCmd((0x7 & kSysEmuMinionShiresMask));         /* Shire 0-2 (if available) */
  launchAddVectorKernelListCmd((0xF & kSysEmuMinionShiresMask));         /* Shire 0-4 (if available) */
  launchAddVectorKernelListCmd((0xFF & kSysEmuMinionShiresMask));        /* Shire 0-8 (if available) */
  launchAddVectorKernelListCmd((0xFFFF & kSysEmuMinionShiresMask));      /* Shire 0-16 (if available) */
  launchAddVectorKernelListCmd((0x1FFFFFFFF & kSysEmuMinionShiresMask)); /* Shire 0-32 (if available) */
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchUberKernelDMAList_PositiveTesting_4_14) {
  launchUberKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchEmptyKernelDMAList_PositiveTesting_4_15) {
  launchEmptyKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchExceptionKernelDMAList_NegativeTesting_4_16) {
  launchExceptionKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, abortHangKernelDMAList_PositiveTesting_4_17) {
  launchHangKernelListCmd(kSysEmuMinionShiresMask, true);
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, allTestsConsecutivelyKernelDMAList) {
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask);
  launchUberKernelListCmd(kSysEmuMinionShiresMask);
  launchEmptyKernelListCmd(kSysEmuMinionShiresMask);
  launchExceptionKernelListCmd(kSysEmuMinionShiresMask);
  launchHangKernelListCmd(kSysEmuMinionShiresMask, true);
}


TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, launchAddVectorKernelDMAList_CM_UModeTesting_6_11) {
  // provide the new kernel elf that uses the cm-umode lib
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask, KernelTypes::CMUMODE_KERNEL_TYPE);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, kernelLaunchCmd_InvalidAddressNegativeTesting_6_3) {
  kernelLaunchCmd_InvalidAddressNegativeTesting(kSysEmuMinionShiresMask);
}
// TODO: Enable this test once runtime has switched to using new kernel launch command
TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, DISABLED_kernelLaunchCmd_InvalidArgSizeNegativeTesting_6_4) {
  kernelLaunchCmd_InvalidArgSizeNegativeTesting(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, kernelLaunchCmd_ExceptionKernelNegativeTesting_6_5) {
  launchExceptionKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, kernelLaunchCmd_AbortKernelNegativeTesting_6_7) {
  launchHangKernelListCmd(kSysEmuMinionShiresMask, true);
}

TEST_F(TestDevOpsApiFuncKernelCmdsPcieSysEmu, kernelLaunchCmd_InvalidShireMaskNegativeTesting_6_8) {
  // the device supports shires from 0 to 32. Give a mask greater than this for negative testing
  kernelLaunchCmd_InvalidShireMaskNegativeTesting(0xFFFFFFFFFF);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
