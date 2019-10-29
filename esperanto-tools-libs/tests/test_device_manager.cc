//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Core/DeviceManager.h"
#include "Core/DeviceTarget.h"
#include "Device/TargetSysEmu.h"
#include "Support/DeviceGuard.h"

#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <thread>

using namespace std;

using namespace et_runtime::device;

TEST(DeviceManager, deviceFactory) {
  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_grpc"));
  ASSERT_TRUE(absl::GetFlag(FLAGS_dev_target).dev_target.find("sysemu_grpc") !=
              string::npos);
  auto target_type = DeviceTarget::deviceToCreate();
  auto dev_target = DeviceTarget::deviceFactory(target_type, 0);
  ASSERT_TRUE(dynamic_cast<TargetSysEmu *>(dev_target.get()) != nullptr);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  google::SetCommandLineOption("GLOG_logtostderr", "1");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
