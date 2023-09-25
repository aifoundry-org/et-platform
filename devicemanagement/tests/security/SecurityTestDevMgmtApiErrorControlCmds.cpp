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

class SecurityTestDevMgmtApiErrorControlCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    initEventProcessor();
    controlTraceLogging();
    initDevErrorEvent();
  }
  void TearDown() override {
    cleanupEventProcessor();
    checkDevErrorEvent();
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidInputBuffer) {
  setDDRECCCountInvalidInputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidInputSize) {
  setDDRECCCountInvalidInputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidOutputSize) {
  setDDRECCCountInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidDeviceNode) {
  setDDRECCCountInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidHostLatency) {
  setDDRECCCountInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidDeviceLatency) {
  setDDRECCCountInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setDDRECCCountInvalidOutputBuffer) {
  setDDRECCCountInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCCountInvalidInputBuffer) {
  setPCIEECCCountInvalidInputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidInputSize) {
  setPCIEECCountInvalidInputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidOutputSize) {
  setPCIEECCountInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidDeviceNode) {
  setPCIEECCountInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidHostLatency) {
  setPCIEECCountInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidDeviceLatency) {
  setPCIEECCountInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setPCIEECCountInvalidOutputBuffer) {
  setPCIEECCountInvalidOutputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCCountInvalidInputBuffer) {
  setSRAMECCCountInvalidInputBuffer(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCCountInvalidInputSize) {
  setSRAMECCCountInvalidInputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCountInvalidOutputSize) {
  setSRAMECCountInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCountInvalidDeviceNode) {
  setSRAMECCountInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCountInvalidHostLatency) {
  setSRAMECCountInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCountInvalidDeviceLatency) {
  setSRAMECCountInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiErrorControlCmds, setSRAMECCountInvalidOutputBuffer) {
  setSRAMECCountInvalidOutputBuffer(false /* Multiple Devices */);
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
