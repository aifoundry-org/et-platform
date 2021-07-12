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

using namespace dev::dl_tests;

class TestDevOpsApiStressDmaCmdsPcie : public TestDevOpsApiDmaCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressSize_2_1) {
  dataWRStressSize_2_1(25);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressSpeed_2_2) {
  dataWRStressSpeed_2_2(25);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressChannelsSingleDeviceSingleQueue_2_3) {
  dataWRStressChannels(true, true, 1000);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRStressChannelsSingleDeviceMultiQueue_2_4) {
  dataWRStressChannels(true, false, 1000);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dmaList1kWrAndRdSingleDeviceSingleQueue_2_5) {
  dmaListWrAndRd(true /* single device */, true /* single queue */, 1000);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dmaList1kWrAndRdSingleDeviceMultiQueue_2_6) {
  dmaListWrAndRd(true /* single device */, false /* multiple queues */, 1000);
}

TEST_F(TestDevOpsApiStressDmaCmdsPcie, dataWRCmdSingleWriteMultipleReads_2_7) {
  dataWRCmdSingleWriteMultipleReads(50);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
