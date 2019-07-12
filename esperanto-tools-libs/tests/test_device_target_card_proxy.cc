//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Device/TargetCardProxy.h"
#include "gtest/gtest.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace std;

namespace {

TEST(TargetCardProxy, StartStopSysEmu) {
  CardProxyTarget device(0);

  // Start the simulator
  ASSERT_TRUE(device.init());
  this_thread::sleep_for(chrono::seconds(1));
  // Stop the simulator
  ASSERT_TRUE(device.deinit());
}

extern std::string FLAGS_dev_target;

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

} // namespace
