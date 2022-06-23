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

class IntegrationTestDevMgmtApiCmds : public TestDevMgmtApiSyncCmds {
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

TEST_F(IntegrationTestDevMgmtApiCmds, serializeAccessMgmtNode) {
  serializeAccessMgmtNode(false);
}

TEST_F(IntegrationTestDevMgmtApiCmds, getDeviceErrorEvents) {
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen:
  // When executed back to back along with ops node tests, dmesg was inundated with
  // 'Minion Runtime Error' events and could not read other error events probably
  // because the dmesg buffer wrapped around.
  // I0624 08:52:49.750947 248176 TestDevMgmtApiSyncCmds.cpp:1750] ET [DM][TH:140591309539968]: waiting done, starting events verification...
  // I0624 08:52:49.752763 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'PCIe Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.752928 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'PCIe Un-Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.752971 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'DRAM Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.752995 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'DRAM Un-Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753019 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'SRAM Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753041 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'SRAM Un-Correctable Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753062 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'Power Management IC Errors' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753085 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'Minion Runtime Error' 1490 time(s)
  // I0624 08:52:49.753087 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'Minion Runtime Hang' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753108 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'SP Runtime Error' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // I0624 08:52:49.753129 248176 TestDevMgmtApiSyncCmds.cpp:1769] ET [DM][TH:140591309539968]: matched 'SP Runtime Exception' 0 time(s)
  // Expected: (err_count[i]) >= (1), actual: 0 vs 1
  // Expected equality of these values:
  //   result
  //     Which is: 1
  //   max_err_types
  //     Which is: 11
  if (getTestTarget() != Target::Silicon) {
    getDeviceErrorEvents(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiCmds, setTraceControl) {
  DV_LOG(INFO) << "setTraceControl: verifying disable trace control command";
  setTraceControl(false /* Multiple devices */, device_mgmt_api::TRACE_CONTROL_TRACE_DISABLE);
  DV_LOG(INFO) << "setTraceControl: verifying enabling trace (dump to UART) control command";
  setTraceControl(false /* Multiple devices */,
                  device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE | device_mgmt_api::TRACE_CONTROL_TRACE_UART_ENABLE);
  DV_LOG(INFO) << "setTraceControl: verifying enabling trace (dump to trace buffer) control command";
  setTraceControl(false /* Multiple devices */, device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE);
}

TEST_F(IntegrationTestDevMgmtApiCmds, setTraceConfigure) {
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                    device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);

  /* Restore the logging level back */
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                    device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
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
