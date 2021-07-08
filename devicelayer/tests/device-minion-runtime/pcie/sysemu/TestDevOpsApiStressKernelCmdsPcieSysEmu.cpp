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

class TestDevOpsApiStressKernelCmdsPcieSysEmu : public TestDevOpsApiKernelCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelLaunchCmdsSingleDeviceSingleQueue_3_1) {
  backToBackSameKernelLaunchCmds_3_1(true, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelLaunchCmdsSingleDeviceMultiQueue_3_2) {
  backToBackSameKernelLaunchCmds_3_1(true, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackDifferentKernelLaunchCmdsSingleDeviceSingleQueue_3_5) {
  backToBackDifferentKernelLaunchCmds_3_2(true, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackDifferentKernelLaunchCmdsSingleDeviceMultiQueue_3_6) {
  backToBackDifferentKernelLaunchCmds_3_2(true, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackEmptyKernelLaunch_3_9) {
  backToBackEmptyKernelLaunch_3_3(100, kSysEmuMinionShiresMask, false);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackEmptyKernelLaunchFlushL3_3_10) {
  backToBackEmptyKernelLaunch_3_3(100, kSysEmuMinionShiresMask, true);
}

/**********************************************************
 *                                                         *
 *          Kernel DMA LIST Tests                          *
 *                                                         *
 **********************************************************/

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_11) {
  backToBackSameKernelLaunchListCmds(true, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelDMAListLaunchCmdsSingleDeviceMultiQueue_3_12) {
  backToBackSameKernelLaunchListCmds(true, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_13) {
  backToBackSameKernelLaunchListCmds(false, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackSameKernelDMAListLaunchCmdsMultiDeviceMultiQueue_3_14) {
  backToBackSameKernelLaunchListCmds(false, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu,
       backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceSingleQueue_3_15) {
  backToBackDifferentKernelLaunchListCmds(true, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu,
       backToBackDifferentKernelDMAListLaunchCmdsSingleDeviceMultiQueue_3_16) {
  backToBackDifferentKernelLaunchListCmds(true, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceSingleQueue_3_17) {
  backToBackDifferentKernelLaunchListCmds(false, true, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu,
       backToBackDifferentKernelDMAListLaunchCmdsMultiDeviceMultiQueue_3_18) {
  backToBackDifferentKernelLaunchListCmds(false, false, 100, kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackEmptyKernelDMAListLaunch_3_19) {
  backToBackEmptyKernelLaunchListCmds(true, true, 100, kSysEmuMinionShiresMask, false);
}

TEST_F(TestDevOpsApiStressKernelCmdsPcieSysEmu, backToBackEmptyKernelDMAListLaunchFlushL3_3_20) {
  backToBackEmptyKernelLaunchListCmds(true, true, 100, kSysEmuMinionShiresMask, true);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
