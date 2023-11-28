//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModulePowerState) {
  getModulePowerState(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setAndGetModuleStaticTDPLevel) {
  setAndGetModuleStaticTDPLevel(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setAndGetModuleTemperatureThreshold) {
  setAndGetModuleTemperatureThreshold(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleResidencyThrottleState) {
  getModuleResidencyThrottleState(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleUptime) {
  getModuleUptime(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModulePower) {
  getModulePower(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getAsicVoltage) {
  getAsicVoltage(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleVoltage) {
  getModuleVoltage(false /* Multiple devices */);
}

// TODO: SW-16538: Enable back when fixed
TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, DISABLED_setAndGetModuleVoltage) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    FLAGS_enable_trace_dump = false;
    return;
  }
  setAndGetModuleVoltage(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleCurrentTemperature) {
  getModuleCurrentTemperature(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleResidencyPowerState) {
  getModuleResidencyPowerState(false /* Multiple devices */);
}

/* TODO: SW-19495: Enable the test once the issue is resolved */
TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, DISABLED_setModuleActivePowerManagement) {
  setModuleActivePowerManagement(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setThrottlePowerStatus) {
  setThrottlePowerStatus(false /* Multiple Devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setAndGetModuleFrequency) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    FLAGS_enable_trace_dump = false;
    return;
  }
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
    setAndGetModuleFrequency(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
