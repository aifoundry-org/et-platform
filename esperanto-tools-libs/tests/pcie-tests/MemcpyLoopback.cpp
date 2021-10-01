//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "Utils.h"
#include "runtime/IRuntime.h"
#include <device-layer/IDeviceLayer.h>
using namespace rt;

TEST_F(Fixture, memcpy_64B_5_EXEC_1_ST_20_TH) {
  run_stress_mem(runtime_.get(), 64, 5, 1, 20, false);
}

int main(int argc, char** argv) {
  Fixture::sMode = Fixture::Mode::PCIE;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}