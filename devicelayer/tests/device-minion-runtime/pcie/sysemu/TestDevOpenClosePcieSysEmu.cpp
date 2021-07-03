//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "testHelper/TestDevOpsApi.h"

/*
 * Test Labels: PCIE-SYSEMU, OPS, FUNCTIONAL, UNIT
 */

using namespace dev::dl_tests;

class TestDevOpenClosePcieSysEmu : public TestDevOpsApi {};

TEST_F(TestDevOpenClosePcieSysEmu, instantiateIDeviceLayer_1_1) {
  execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
  initTestHelperPcie();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
