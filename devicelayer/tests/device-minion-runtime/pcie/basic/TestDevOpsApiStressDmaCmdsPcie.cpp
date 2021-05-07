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

class TestDevOpsApiStressDmaCmdsPcie : public TestDevOpsApiDmaCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    // Launch PCIE through IDevice Abstraction
    devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer(true, false);

    resetMemPooltoDefault();
  }
};

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressSize_2_1) {
  dataWRStressSize_2_1(25);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressSpeed_2_2) {
  dataWRStressSpeed_2_2(25);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressChannelsSingleQueue_2_3) {
  dataWRStressChannelsSingleQueue_2_3(1000);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressChannelsMultiQueue_2_4) {
  dataWRStressChannelsMultiQueue_2_4(1000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
