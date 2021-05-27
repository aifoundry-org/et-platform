//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "testHelper/TestDevOpsApi.h"

class TestDevOpsApiBasicCmds : public TestDevOpsApi {
protected:
  /* Basic Testing Functions */
  void echoCmd_PositiveTest_2_1();
  void devCompatCmd_PositiveTest_2_3();
  void devFWCmd_PostiveTest_2_5();
  void devUnknownCmd_NegativeTest_2_7();

  /* Stress Testing Functions */
  void backToBackSameCmdsSingleDeviceSingleQueue_1_1(int numOfCmds);
  void backToBackSameCmdsSingleDeviceMultiQueue_1_2(int numOfCmds);
  void backToBackDiffCmdsSingleDeviceSingleQueue_1_3(int numOfCmds);
  void backToBackDiffCmdsSingleDeviceMultiQueue_1_4(int numOfCmds);
  void backToBackSameCmdsMultiDeviceMultiQueue_1_5(int numOfCmds);
  void backToBackDiffCmdsMultiDeviceMultiQueue_1_6(int numOfCmds);
};
