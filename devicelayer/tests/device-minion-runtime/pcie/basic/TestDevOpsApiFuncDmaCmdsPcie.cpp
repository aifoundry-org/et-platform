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

    // Launch PCIE through IDevice Abstraction
    devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer(true, false);

    initTestHelper();
  }
};

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmd_PositiveTesting_3_1) {
  dataRWCmd_PositiveTesting_3_1();
}

TEST_F(TestDevOpsApiFuncDmaCmdsPcie, dataRWCmdWithBasicCmds_3_4) {
  dataRWCmdWithBasicCmds_3_4();
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

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
