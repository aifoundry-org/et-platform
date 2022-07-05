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
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen with V2/V3 card
  // Expected: (module_power->power) != (0), actual: '\0' vs 0
  if (getTestTarget() != Target::Silicon) {
    getModulePower(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, getModuleVoltage) {
  // TODO: 13282: Enable back on Target::Silicon. The test runs fine in the beginning but in
  // longer runs it times out, i.e. no response is received.
  // if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu})) {
    getModuleVoltage(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
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
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen with V2/V3 card
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

TEST_F(FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds, setModuleFrequency) {
  // TODO: SW-13219: V2 cards: Setting 400MHz/400MHz freq results in resetting of host
  // V3 cards: Able to set 400MHz/400MHz freq but fails intermittently after few
  // iterations with following error:
  // Received incorrect rsp status: -15001
  if (getTestTarget() != Target::Silicon) {
    setModuleFrequency(false /* Multiple devices */);
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
