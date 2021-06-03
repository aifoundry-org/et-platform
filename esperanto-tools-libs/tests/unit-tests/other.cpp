//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h" 
#include <device-layer/IDeviceLayerFake.h>
#include "TestUtils.h"
#include "utils.h"
using namespace rt;

namespace {
class TestFixture : public ::Fixture {
public:
  explicit TestFixture() {
    init(std::make_unique<dev::IDeviceLayerFake>());
  }
};
}

TEST_F(TestFixture, checkStackTraceException)  {
  //run a kernelLaunch with bad parameters to check for the exception
  try {
    runtime_->kernelLaunch(StreamId{7}, KernelId{8}, nullptr, 0, 0);
  } catch (const rt::Exception& e) {
    RT_VLOG(HIGH) << e.what();
  }
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}