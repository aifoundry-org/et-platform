//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiDmaCmds.h"
/*
 * Test Labels: PCIE-SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;

class TestDevOpsApiFuncDmaCmdsPcieSysEmu : public TestDevOpsApiDmaCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWCmd_PositiveTesting_3_1) {
  dataRWCmd_PositiveTesting_3_1();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWCmdMixed_3_5) {
  dataRWCmdMixed_3_5();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWCmdMixedWithVarSize_3_6) {
  dataRWCmdMixedWithVarSize_3_6();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWCmdAllChannels_3_7) {
  dataRWCmdAllChannels_3_7();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWCmdWithBarrier_PositiveTesting_3_10) {
  dataRWCmdWithBarrier_PositiveTesting_3_10();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmd_PositiveTesting_3_11) {
  dataRWListCmd_PositiveTesting_3_11();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmd_NegativeTesting_3_12) {
  dataRWListCmd_NegativeTesting_3_12();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmdMixed_3_13) {
  dataRWListCmdMixed();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmdMixedWithVarSize_3_14) {
  dataRWListCmdMixedWithVarSize();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmdAllChannels_3_15) {
  dataRWListCmdAllChannels();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, dataRWListCmdWithBarrier_PositiveTesting_3_16) {
  dataRWListCmdWithBarrier_PositiveTesting();
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncDmaCmdsPcieSysEmu, allTestsConsecutively) {
  dataRWCmd_PositiveTesting_3_1();
  dataRWCmdMixed_3_5();
  dataRWCmdMixedWithVarSize_3_6();
  dataRWCmdAllChannels_3_7();
  dataRWCmdWithBarrier_PositiveTesting_3_10();
  dataRWListCmd_PositiveTesting_3_11();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
