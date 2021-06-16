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

class TestDevOpsApiLoopbackDmaCmdsPcie : public TestDevOpsApiDmaCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dataWRStressSize_2_1) {
  dataWRStressSize_2_1(25);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dataWRStressSpeed_2_2) {
  dataWRStressSpeed_2_2(25);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dataWrRdCmd1kWrAndRdSingleDeviceSingleQueue_2_3) {
  dataWRStressChannelsSingleDeviceSingleQueue_2_3(1000);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dataWrRdCmd1kWrAndRdSingleDeviceMultiQueue_2_4) {
  dataWRStressChannelsSingleDeviceMultiQueue_2_4(1000);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dataWrRdCmd1kWrAndRdMultiDeviceMultiQueue_2_5) {
  dataWRStressChannelsMultiDeviceMultiQueue_2_5(1000);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dmaList50kWrAndRdSingleDeviceSingleQueue_2_6) {
  dmaListWrAndRd(true /* single device */, true /* single queue */, 50000);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dmaList50kWrAndRdSingleDeviceMultiQueue_2_7) {
  dmaListWrAndRd(true /* single device */, false /* multiple queues */, 50000);
}

TEST_F(TestDevOpsApiLoopbackDmaCmdsPcie, dmaList50kWrAndRdMultiDeviceMultiQueue_2_8) {
  dmaListWrAndRd(false /* multiple devices */, false /* multiple queues */, 50000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_loopback_driver) {
    // Loopback driver does not support trace
    FLAGS_enable_trace_dump = false;
  }
  return RUN_ALL_TESTS();
}
