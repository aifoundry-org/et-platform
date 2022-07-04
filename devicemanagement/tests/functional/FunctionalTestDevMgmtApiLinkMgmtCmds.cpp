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

class FunctionalTestDevMgmtApiLinkMgmtCmds : public TestDevMgmtApiSyncCmds {
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

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, getPCIEECCUECCCount) {
  getPCIEECCUECCCount(false /* Multiple devices */);
}

/*
TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, setPCIEReset) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)(devLayer_.get());

  const uint32_t input_size = sizeof(device_mgmt_api::pcie_reset_e);
  const char input_buff[input_size] = {device_mgmt_api::PCIE_RESET_FLR};

  //Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
  const uint32_t output_size = sizeof(uint32_t);
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RESET, input_buff, input_size,
                              output_buff, output_size, hst_latency.get(), dev_latency.get(), 2000),
            device_mgmt_api::DM_STATUS_SUCCESS);

  ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
}
*/

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, getDDRECCUECCCount) {
  getDDRECCUECCCount(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, getSRAMECCUECCCount) {
  getSRAMECCUECCCount(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, getDDRBWCounter) {
  getDDRBWCounter(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, setPCIELinkSpeed) {
  // TODO: SW-13272: Enable it back for Target::Silicon
  if (getTestTarget() != Target::Silicon) {
    setPCIELinkSpeed(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, setPCIELaneWidth) {
  setPCIELaneWidth(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiLinkMgmtCmds, setPCIERetrainPhy) {
  setPCIERetrainPhy(false /* Multiple devices */);
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
