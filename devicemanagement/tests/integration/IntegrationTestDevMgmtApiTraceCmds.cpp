//******************************************************************************
// Copyright (C) 2022 Esperanto Technologies Inc.
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

class IntegrationTestDevMgmtApiTraceCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
  }
  void TearDown() override {
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
  // TODO: SW-13138 Workaround, should be removed
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ASSERT_TRUE(extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP));

  /* Restore the logging level back */
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                    device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
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

// TODO: SW-12030 This test can be enabled after adding sampler threads in SP
TEST_F(IntegrationTestDevMgmtApiTraceCmds, getSPStatsTraceBuffer) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    /* Only verifying pulling of CM trace buffer from device, trace data validation is being done in firmware tests */
    ASSERT_TRUE(extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSPStats));
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
