//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiTraceCmds.h"

/*
 * Test Labels: PCIE, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;

class TestDevOpsApiFuncTraceCmdsPcie : public TestDevOpsApiTraceCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiFuncTraceCmdsPcie, traceCtrlAndExtractMMFwData_5_1) {
  traceCtrlAndExtractMMFwData_5_1();
}

TEST_F(TestDevOpsApiFuncTraceCmdsPcie, traceCtrlAndExtractCMFwData_5_2) {
  traceCtrlAndExtractCMFwData_5_2();
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 */
TEST_F(TestDevOpsApiFuncTraceCmdsPcie, allTestsConsecutively) {
  traceCtrlAndExtractMMFwData_5_1();
  traceCtrlAndExtractCMFwData_5_2();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
