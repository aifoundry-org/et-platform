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


class TestDevMgmtApiFuncSyncCmdsPcie : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
  }
  void TearDown() override {
    extractAndPrintTraceData();
     if (handle_ != nullptr) {
       dlclose(handle_);
    }
  }

};

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICChipRevision_1_4) {
  getASICChipRevision_1_4(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10(false /* Multiple devices */);
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePowerState_1_11) {
  getModulePowerState_1_11(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleUptime_1_15) {
  getModuleUptime_1_15(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePower_1_16) {
  getModulePower_1_16(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleVoltage_1_17) {
  getModuleVoltage_1_17(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleCurrentTemperature_1_18) {
  getModuleCurrentTemperature_1_18(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxMemoryErrors_1_20) {

  getModuleMaxMemoryErrors_1_20(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxDDRBW_1_21) {
  getModuleMaxDDRBW_1_21(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleResidencyPowerState_1_22) {
  getModuleResidencyPowerState_1_22(false /* Multiple devices */);
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetPCIEECCThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26(false /* Multiple devices */);
}

// Link Management
/*
TEST_F(TestDevMgmtApiSyncCmds, test_DM_CMD_SET_PCIE_RESET) {
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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDDRECCUECCCount_1_27) {
  getDDRECCUECCCount_1_27(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDDRBWCounter_1_29) {
  getDDRBWCounter_1_29(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleActivePowerManagement_1_62) {
  setModuleActivePowerManagement_1_62(false /* Multiple devices */);
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICFrequencies_1_33) {
  getASICFrequencies_1_33(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDRAMBW_1_34) {
  getDRAMBW_1_34(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDRAMCapacityUtilization_1_35) {
  getDRAMCapacityUtilization_1_35(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICUtilization_1_37) {
  getASICUtilization_1_37(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICStalls_1_38) {
  getASICStalls_1_38(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICLatency_1_39) {
  getASICLatency_1_39(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getMMErrorCount_1_40) {
  getMMErrorCount_1_40(false /* Multiple devices */);
}

// Test serial access
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, serializeAccessMgmtNode_1_43) {
  serializeAccessMgmtNode_1_43(false);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, isUnsupportedService_1_45) {
  isUnsupportedService_1_45(false /* Multiple devices */);
}

/*TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setSpRootCertificate_1_46) {
  setSpRootCertificate_1_46(false);
}*/

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setTraceControl_1_47) {
  setTraceControl_1_47(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setTraceConfigure_1_48) {
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getTraceBuffer_1_49) {
  setTraceControl_1_47(false /* Multiple devices */);
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);
  setAndGetModuleStaticTDPLevel_1_12(false /* Multiple devices */);
  getTraceBuffer_1_49(false /* Multiple devices */);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleActivePowerManagementRange_1_50) {
  setModuleActivePowerManagementRange_1_50(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleSetTemperatureThresholdRange_1_51) {
  setModuleSetTemperatureThresholdRange_1_51(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleStaticTDPLevelRange_1_52) {
  setModuleStaticTDPLevelRange_1_52(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleActivePowerManagementRangeInvalidInputSize_1_55) {
  setModuleActivePowerManagementRangeInvalidInputSize_1_55(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureNameInvalidOutputSize_1_56) {
  getModuleManufactureNameInvalidOutputSize_1_56(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureNameInvalidDeviceNode_1_57) {
  getModuleManufactureNameInvalidDeviceNode_1_57(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureNameInvalidHostLatency_1_58) {
  getModuleManufactureNameInvalidHostLatency_1_58(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureNameInvalidDeviceLatency_1_59) {
  getModuleManufactureNameInvalidDeviceLatency_1_59(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureNameInvalidOutputBuffer_1_60) {
  getModuleManufactureNameInvalidOutputBuffer_1_60(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setModuleActivePowerManagementRangeInvalidInputBuffer_1_61) {
  setModuleActivePowerManagementRangeInvalidInputBuffer_1_61(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELinkSpeedToInvalidLinkSpeed_1_64) {
  setPCIELinkSpeedToInvalidLinkSpeed_1_64(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELaneWidthToInvalidLaneWidth_1_65) {
  setPCIELaneWidthToInvalidLaneWidth_1_65(false /* Multiple Devices */);
}


int main(int argc, char** argv) {
  logging::LoggerDefault loggerDefault_;
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_loopback_driver) {
    // Loopback driver does not support trace
    FLAGS_enable_trace_dump = false;
  }
  return RUN_ALL_TESTS();
}
