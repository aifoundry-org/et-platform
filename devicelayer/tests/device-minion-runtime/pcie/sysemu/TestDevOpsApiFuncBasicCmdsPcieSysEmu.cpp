//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiBasicCmds.h"

/*
 * Test Labels: PCIE-SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;

class TestDevOpsApiFuncBasicCmdsPcieSysEmu : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiFuncBasicCmdsPcieSysEmu, echoCmd_PositiveTest_2_1) {
  echoCmd_PositiveTest_2_1();
}

TEST_F(TestDevOpsApiFuncBasicCmdsPcieSysEmu, devCompatCmd_PositiveTest_2_3) {
  devCompatCmd_PositiveTest_2_3();
}

TEST_F(TestDevOpsApiFuncBasicCmdsPcieSysEmu, devFWCmd_PostiveTest_2_5) {
  devFWCmd_PostiveTest_2_5();
}

TEST_F(TestDevOpsApiFuncBasicCmdsPcieSysEmu, devUnknownCmd_NegativeTest_2_7) {
  devUnknownCmd_NegativeTest_2_7();
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncBasicCmdsPcieSysEmu, allTestsConsecutively) {
  echoCmd_PositiveTest_2_1();
  devCompatCmd_PositiveTest_2_3();
  devFWCmd_PostiveTest_2_5();
  devUnknownCmd_NegativeTest_2_7();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
