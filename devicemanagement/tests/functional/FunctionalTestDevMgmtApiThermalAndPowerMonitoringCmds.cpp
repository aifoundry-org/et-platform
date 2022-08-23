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
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging();
  }
  void TearDown() override {
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
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

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleVoltage) {
  getModuleVoltage(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleCurrentTemperature) {
  getModuleCurrentTemperature(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleResidencyPowerState) {
  getModuleResidencyPowerState(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setModuleActivePowerManagement) {
  setModuleActivePowerManagement(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setThrottlePowerStatus) {
  // TODO: SW-13953: Enable back on Target::Silicon, following failure is seen on silicon
  // No SP trace event found!
  // The txt trace file when failure occurs has no logged traces:
  // bash-4.2$ cat devtrace/txt_files/dev0_traces.txt
  //
  // FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds.setThrottlePowerStatus
  // -> SP Traces
  if (getTestTarget() != Target::Silicon) {
    setThrottlePowerStatus(false /* Multiple Devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

// TODO: SW-13952: Intermittent segmentation fault occuring
TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, DISABLED_setAndGetModuleFrequency) {
  // TODO: SW-13952: Enable back on Target::Silicon. It fails intermittently after few iterations
  // with following error:
  // Received incorrect rsp status: -15001
  // if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu})) {
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
