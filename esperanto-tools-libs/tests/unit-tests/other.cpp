//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "Utils.h"
#include "runtime/IRuntime.h"
using namespace rt;

TEST_F(RuntimeFixture, checkStackTraceException) {
  //run a kernelLaunch with bad parameters to check for the exception
  try {
    runtime_->kernelLaunch(StreamId{7}, KernelId{8}, nullptr, 0, 0);
  } catch (const rt::Exception& e) {
    RT_VLOG(HIGH) << e.what();
  }
}

int main(int argc, char** argv) {
  RuntimeFixture::sDlType = RuntimeFixture::DeviceLayerImp::FAKE;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}