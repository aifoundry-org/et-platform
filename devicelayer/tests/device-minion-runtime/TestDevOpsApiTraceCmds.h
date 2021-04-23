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

class TestDevOpsApiTraceCmds : public TestDevOpsApi {
protected:
  /* Trace testing */
  void traceCtrlAndExtractMMFwData_5_1();
  void traceCtrlAndExtractCMFwData_5_2();

private:

  void printMMTraceStringData(std::vector<uint8_t> & traceBuf);
  void printCMTraceStringData(std::vector<uint8_t> & traceBuf);
};
