//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiBasicCmds.h"

/*
 * Test Labels: NIGHTLY, PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiNightlyBasicCmdsPcie : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    // Launch PCIE through IDevice Abstraction
    devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer();

    resetMemPooltoDefault();
  }
};

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack3kSameCmds_1_3) {
  backToBackSameCmds_1_1(3000);
}

/* Commenting out till FC issues has been removed
TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack9kDiffCmds_1_4) {
  backToBackDiffCmds_1_2(3000);
}
*/

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
