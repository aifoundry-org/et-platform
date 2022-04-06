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
    controlTraceLogging(false);
  }
  void TearDown() override {
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    controlTraceLogging(true);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(IntegrationTestDevMgmtApiCmds, serializeAccessMgmtNode) {
  // TODO: SW-10585: Enable back these tests on silicon currently service not functional
  if (getTestTarget() == Target::Silicon) {
    std::exit(EXIT_SUCCESS);
  }
  serializeAccessMgmtNode(false);
}

TEST_F(IntegrationTestDevMgmtApiCmds, getDeviceErrorEvents) {
  // TODO: SW-10585: Enable back these tests on silicon currently service not functional
  if (getTestTarget() == Target::Silicon) {
    std::exit(EXIT_SUCCESS);
  }

  if (getTestTarget() != Target::Loopback) {
    getDeviceErrorEvents(false /* Multiple devices */);
  } else {
    DM_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiCmds, DISABLED_resetMM) {
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
    resetMM(false);
  } else {
    DM_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiCmds, setTraceControl) {
  DM_LOG(INFO) << "setTraceControl: verifying disable trace control command";
  setTraceControl(false /* Multiple devices */, device_mgmt_api::TRACE_CONTROL_TRACE_DISABLE);
  DM_LOG(INFO) << "setTraceControl: verifying enabling trace (dump to UART) control command";
  setTraceControl(false /* Multiple devices */,
                  device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE | device_mgmt_api::TRACE_CONTROL_TRACE_UART_ENABLE);
  DM_LOG(INFO) << "setTraceControl: verifying enabling trace (dump to trace buffer) control command";
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
  logging::LoggerDefault loggerDefault_;
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
