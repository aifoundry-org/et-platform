//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/Device.h"
#include "Core/CommandLineOptions.h"
#include "Core/DeviceManager.h"
#include "Core/DeviceTarget.h"
#include "Device/TargetCardProxy.h"
#include "Support/DeviceGuard.h"

#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <thread>

using namespace std;

using namespace et_runtime::device;

namespace {

TEST(DeviceManager, deviceFactory) {
  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_card_proxy"));
  ASSERT_TRUE(absl::GetFlag(FLAGS_dev_target).dev_target.find("sysemu_card_proxy") != string::npos);
  auto target_type = DeviceTarget::deviceToCreate();
  auto dev_target = DeviceTarget::deviceFactory(target_type, "test_path");
  ASSERT_TRUE(dynamic_cast<CardProxyTarget *>(dev_target.get()) != nullptr);
}


TEST(DeviceManager, RegisteAndAccessDevice) {

  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_card_proxy"));
  auto device_manager = et_runtime::getDeviceManager();
  auto ret_value = device_manager->registerDevice(0);

  ASSERT_TRUE(ret_value);
  auto dev = ret_value.get();

  ASSERT_EQ(dev->init(), etrtSuccess);
  ASSERT_EQ(dev->resetDevice(), etrtSuccess);
}


int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  testing::InitGoogleTest(&argc, argv);

  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_card_proxy"));
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

} // namespace
