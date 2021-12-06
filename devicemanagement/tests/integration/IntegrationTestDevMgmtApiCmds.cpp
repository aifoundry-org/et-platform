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
    if (getTestTarget() == Target::Loopback) {
      // Loopback driver does not support trace
      FLAGS_enable_trace_dump = false;
    }
  }
  void TearDown() override {
    extractAndPrintTraceData();
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(IntegrationTestDevMgmtApiCmds, serializeAccessMgmtNode) {
  serializeAccessMgmtNode(false);
}

TEST_F(IntegrationTestDevMgmtApiCmds, getDeviceErrorEvents) {
  if (targetInList({ Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon })) {
    getDeviceErrorEvents(false /* Multiple devices */);
  } else {
    DM_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(IntegrationTestDevMgmtApiCmds, setTraceControl) {
  setTraceControl(false /* Multiple devices */);
}

TEST_F(IntegrationTestDevMgmtApiCmds, setTraceConfigure) {
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);

  /* Restore the logging level back */
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(IntegrationTestDevMgmtApiCmds, getTraceBuffer) {
  setTraceControl(false /* Multiple devices */);
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);
  setAndGetModuleStaticTDPLevel(false /* Multiple devices */);
  getTraceBuffer(false /* Multiple devices */);

  /* Restore the logging level back */
  setTraceConfigure(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(IntegrationTestDevMgmtApiCmds, resetCM) {
  if (targetInList({ Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon })) {
    resetCM(false /* Multiple devices */);
  } else {
    DM_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
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
