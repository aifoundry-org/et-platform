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

class TestDevOpsApiDmaCmds : public TestDevOpsApi {
protected:
  /* DMA Basic Testing Functions */
  void dataRWCmdWithBasicCmds_3_4();
  void dataRWCmdMixed_3_5();
  void dataRWCmdMixedWithVarSize_3_6();
  void dataRWCmdAllChannels_3_7();

  /* DMA Positive Testing Functions */
  void dataRWCmd_PositiveTesting_3_1();
  void dataRWCmdWithBarrier_PositiveTesting_3_10();

  /* DMA Stress Testing Functions */
  void dataWRStressSize_2_1(uint8_t maxExp2);
  void dataWRStressSpeed_2_2(uint8_t maxExp2);
  void dataWRStressChannelsSingleDeviceSingleQueue_2_3(uint32_t numOfLoopbackCmds);
  void dataWRStressChannelsSingleDeviceMultiQueue_2_4(uint32_t numOfLoopbackCmds);
  void dataWRStressChannelsMultiDeviceMultiQueue_2_5(uint32_t numOfLoopbackCmds);
};
