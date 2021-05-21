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

class TestDevOpsApiKernelCmds : public TestDevOpsApi {
protected:
  /* Kernel Functional Tests */
  void launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask);
  void launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask);
  void launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask);
  void launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask);
  void abortHangKernel_PositiveTesting_4_10(uint64_t shire_mask);

  /* Kernel Stress Tests*/
  void backToBackSameKernelLaunchCmds_3_1(uint64_t shire_mask);
  void backToBackDifferentKernelLaunchCmds_3_2(uint64_t shire_mask);
  void backToBackEmptyKernelLaunch_3_3(uint64_t shire_mask, bool flushL3);
  /* Kernel Negative Tests */
  void kernelAbortCmd_InvalidTagIdNegativeTesting_6_2();
};
