//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RPCDevice/TargetSysEmu.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace std;

namespace {

TEST(TargetGRPCSysEmu, StartStopSysEmu) {
  TargetSysEmu device(0);

  // Start the simulator
  ASSERT_TRUE(device.init());
  // Stop the simulator
  ASSERT_TRUE(device.deinit());
}

TEST(TargetGRPCSysEmu, GRPCCardEmu) {
  TargetSysEmu device(0);

  // Start the simulator
  ASSERT_TRUE(device.init());
  // Send memory definition
  uintptr_t addr = 0x8000100000;
  // FIXME we should try tye launch, boot function but those
  // functions have side-effects to the simulator and we do not
  // have a mock kernel to test on the simulator
  // Try to copy data back and forth
  std::array<uint8_t, 10> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  ASSERT_TRUE(device.writeDevMemMMIO(addr, data.size(), data.data()));
  std::array<uint8_t, 10> out_data;
  ASSERT_TRUE(device.readDevMemMMIO(addr, out_data.size(), out_data.data()));
  // Stop the simulator
  ASSERT_TRUE(device.deinit());
  ASSERT_THAT(out_data, ::testing::ElementsAreArray(data));
}

} // namespace
