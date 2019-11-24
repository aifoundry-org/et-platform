//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include "esperanto/runtime/Core/DeviceHelpers.h"
#include "esperanto/runtime/Core/VersionCheckers.h"

#include <thread>
#include <chrono>

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

using namespace et_runtime;

TEST_F(DeviceFWTest, DeviceAPI_GetDevFWVersion) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
  ASSERT_EQ(dev_->init(), etrtSuccess);

  GitVersionChecker git_checker (*dev_);
  auto devfw_hash = git_checker.deviceFWHash();
  ASSERT_TRUE(devfw_hash != 0);
}

// Test the call that retries the DevceAPI version from the Device
TEST_F(DeviceFWTest, DeviceAPI_GetDevAPIVersion) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
  ASSERT_EQ(dev_->init(), etrtSuccess);

  DeviceAPIChecker devapi_checker(*dev_);
  auto success = devapi_checker.getDeviceAPIVersion();
  ASSERT_TRUE(success);
  // Require that we always build the runtime with a target device-fw
  // that the runtime can support
  success = devapi_checker.isDeviceSupported();
  ASSERT_TRUE(success);
}

// Test setting the master minion logging levels
TEST_F(DeviceFWTest, DeviceAPI_SetLogLevelMaster) {
  // Do nothing make sure that the fixture starts/stop the simulator correctly
  ASSERT_EQ(dev_->init(), etrtSuccess);

  SetMasterLogLevel master_level(*dev_);
  auto success = master_level.set_level_critical();
  ASSERT_TRUE(success);
  success = master_level.set_level_error();
  ASSERT_TRUE(success);
  success = master_level.set_level_warning();
  ASSERT_TRUE(success);
  success = master_level.set_level_info();
  ASSERT_TRUE(success);
  success = master_level.set_level_debug();
  ASSERT_TRUE(success);
  success = master_level.set_level_trace();
  ASSERT_TRUE(success);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
