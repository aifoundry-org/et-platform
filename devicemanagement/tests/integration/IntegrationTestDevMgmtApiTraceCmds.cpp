//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class IntegrationTestDevMgmtApiTraceCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    initEventProcessor();
    initDevErrorEvent();
  }
  void TearDown() override {
    cleanupEventProcessor();
    checkDevErrorEvent();
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(IntegrationTestDevMgmtApiTraceCmds, getSpTraceBuffer) {
  if (!FLAGS_enable_trace_dump) {
    DV_LOG(INFO) << "Skipping the test since enable_trace_dump is set to false";
    return;
  }
  setTraceControl(false /* Multiple devices */, device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE);
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                    device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);
  setAndGetModuleStaticTDPLevel(false /* Multiple devices */);

  /* Restore the logging level back */
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                    device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);

  ASSERT_TRUE(extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP));
}

TEST_F(IntegrationTestDevMgmtApiTraceCmds, getMmTraceBuffer) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    /* Only verifying pulling of MM trace buffer from device, trace data validation is being done in firmware tests */
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferMM);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiTraceCmds, getCmTraceBuffer) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    /* Only verifying pulling of CM trace buffer from device, trace data validation is being done in firmware tests */
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferCM);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiTraceCmds, getSPStatsTraceBuffer) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    ASSERT_TRUE(extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSPStats));
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiTraceCmds, dmStatsRunControl) {
  if (targetInList({Target::Silicon})) {
    dmStatsRunControl(false /* Multiple Devices */);
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
