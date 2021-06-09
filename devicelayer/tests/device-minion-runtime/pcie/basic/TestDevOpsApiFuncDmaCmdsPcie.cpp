//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiDmaCmds.h"

/*
 * Test Labels: PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiFuncDmaCmdsPcie : public TestDevOpsApiDmaCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmd_PositiveTesting_3_1) {
  dataRWCmd_PositiveTesting_3_1();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmdMixed_3_5) {
  dataRWCmdMixed_3_5();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmdMixedWithVarSize_3_6) {
  dataRWCmdMixedWithVarSize_3_6();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmdAllChannels_3_7) {
  dataRWCmdAllChannels_3_7();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmdWithBarrier_PositiveTesting_3_10) {
  dataRWCmdWithBarrier_PositiveTesting_3_10();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmd_PositiveTesting_3_11) {
  dataRWListCmd_PositiveTesting_3_11();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmdMixed_3_12) {
  dataRWListCmdMixed();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmdMixedWithVarSize_3_13) {
  dataRWListCmdMixedWithVarSize();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmdAllChannels_3_14) {
  dataRWListCmdAllChannels();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmdWithBarrier_PositiveTesting_3_15) {
  dataRWListCmdWithBarrier_PositiveTesting();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWListCmd_NegativeTesting_3_12) {
  // Skip Test, if loopback driver
  if (FLAGS_loopback_driver) {
    TEST_VLOG(0) << "Skipping: dataRWListCmd_NegativeTesting_3_12, not supported on loopback driver";
    return;
  }
  dataRWListCmd_NegativeTesting_3_12();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
