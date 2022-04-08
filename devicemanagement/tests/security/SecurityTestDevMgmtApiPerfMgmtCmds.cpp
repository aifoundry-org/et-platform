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

class SecurityTestDevMgmtApiPerfMgmtCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging(true);
  }
  void TearDown() override {
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICFrequenciesInvalidOutputSize) {
  getASICFrequenciesInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICFrequenciesInvalidDeviceNode) {
  getASICFrequenciesInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICFrequenciesInvalidHostLatency) {
  getASICFrequenciesInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICFrequenciesInvalidDeviceLatency) {
  getASICFrequenciesInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICFrequenciesInvalidOutputBuffer) {
  getASICFrequenciesInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMBandwidthInvalidOutputSize) {
  getDRAMBandwidthInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMBandwidthInvalidDeviceNode) {
  getDRAMBandwidthInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMBandwidthInvalidHostLatency) {
  getDRAMBandwidthInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMBandwidthInvalidDeviceLatency) {
  getDRAMBandwidthInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMBandwidthInvalidOutputBuffer) {
  getDRAMBandwidthInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilizationInvalidOutputSize) {
  getDRAMCapacityUtilizationInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilizationInvalidDeviceNode) {
  getDRAMCapacityUtilizationInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilizationInvalidHostLatency) {
  getDRAMCapacityUtilizationInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilizationInvalidDeviceLatency) {
  getDRAMCapacityUtilizationInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilizationInvalidOutputBuffer) {
  getDRAMCapacityUtilizationInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilizationInvalidOutputSize) {
  getASICPerCoreDatapathUtilizationInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilizationInvalidDeviceNode) {
  getASICPerCoreDatapathUtilizationInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilizationInvalidHostLatency) {
  getASICPerCoreDatapathUtilizationInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilizationInvalidDeviceLatency) {
  getASICPerCoreDatapathUtilizationInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilizationInvalidOutputBuffer) {
  getASICPerCoreDatapathUtilizationInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICUtilizationInvalidOutputSize) {
  getASICUtilizationInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICUtilizationInvalidDeviceNode) {
  getASICUtilizationInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICUtilizationInvalidHostLatency) {
  getASICUtilizationInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICUtilizationInvalidDeviceLatency) {
  getASICUtilizationInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICUtilizationInvalidOutputBuffer) {
  getASICUtilizationInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICStallsInvalidOutputSize) {
  getASICStallsInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICStallsInvalidDeviceNode) {
  getASICStallsInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICStallsInvalidHostLatency) {
  getASICStallsInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICStallsInvalidDeviceLatency) {
  getASICStallsInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICStallsInvalidOutputBuffer) {
  getASICStallsInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICLatencyInvalidOutputSize) {
  getASICLatencyInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICLatencyInvalidDeviceNode) {
  getASICLatencyInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICLatencyInvalidHostLatency) {
  getASICLatencyInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICLatencyInvalidDeviceLatency) {
  getASICLatencyInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiPerfMgmtCmds, getASICLatencyInvalidOutputBuffer) {
  getASICLatencyInvalidOutputBuffer(false /* Multiple Devices */);
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
