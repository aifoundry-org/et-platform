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

class TestDevMgmtApiFuncSyncCmdsPcieFullBoot : public TestDevMgmtApiSyncCmds {
public:
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
  }

  void TearDown() override {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureName_1_1) {
  getModuleManufactureName_1_1(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModulePartNumber_1_2) {
  getModulePartNumber_1_2(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleSerialNumber_1_3) {
  getModuleSerialNumber_1_3(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICChipRevision_1_4) {
  getASICChipRevision_1_4(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMemorySizeMB_1_6) {
  getModuleMemorySizeMB_1_6(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleRevision_1_7) {
  getModuleRevision_1_7(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleFormFactor_1_8) {
  getModuleFormFactor_1_8(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10(false /* Multiple devices */);
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetModulePowerState_1_11) {
  setAndGetModulePowerState_1_11(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleUptime_1_15) {
  getModuleUptime_1_15(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModulePower_1_16) {
  getModulePower_1_16(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleVoltage_1_17) {
  getModuleVoltage_1_17(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleCurrentTemperature_1_18) {
  getModuleCurrentTemperature_1_18(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMaxMemoryErrors_1_20) {

  getModuleMaxMemoryErrors_1_20(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleMaxDDRBW_1_21) {
  getModuleMaxDDRBW_1_21(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleResidencyPowerState_1_22) {
  getModuleResidencyPowerState_1_22(false /* Multiple devices */);
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setAndGetPCIEECCThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getDDRECCUECCCount_1_27) {
  getDDRECCUECCCount_1_27(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getDDRBWCounter_1_29) {
  getDDRBWCounter_1_29(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32(false /* Multiple devices */);
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICFrequencies_1_33) {
  getASICFrequencies_1_33(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getDRAMBW_1_34) {
  getDRAMBW_1_34(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getDRAMCapacityUtilization_1_35) {
  getDRAMCapacityUtilization_1_35(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICUtilization_1_37) {
  getASICUtilization_1_37(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICStalls_1_38) {
  getASICStalls_1_38(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getASICLatency_1_39) {
  getASICLatency_1_39(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getMMErrorCount_1_40) {
  getMMErrorCount_1_40(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getFWBootstatus_1_41) {
  getFWBootstatus_1_41(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleFWRevision_1_42) {
  getModuleFWRevision_1_42(false /* Multiple devices */);
}
// Test serial access
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, serializeAccessMgmtNode_1_43) {
  serializeAccessMgmtNode_1_43(false);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, isUnsupportedService_1_45) {
  isUnsupportedService_1_45(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setTraceControl_1_47) {
  setTraceControl_1_47(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setTraceConfigure_1_48) {
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getTraceBuffer_1_49) {
  setTraceControl_1_47(false /* Multiple devices */);
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);
  setAndGetModuleStaticTDPLevel_1_12(false /* Multiple devices */);
  getTraceBuffer_1_49(false /* Multiple devices */);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setModulePowerStateRange_1_50) {
  setModulePowerStateRange_1_50(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setModuleSetTemperatureThresholdRange_1_51) {
  setModuleSetTemperatureThresholdRange_1_51(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setModuleStaticTDPLevelRange_1_52) {
  setModuleStaticTDPLevelRange_1_52(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setModulePowerStateRangeInvalidInputSize_1_55) {
  setModulePowerStateRangeInvalidInputSize_1_55(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureNameInvalidOutputSize_1_56) {
  getModuleManufactureNameInvalidOutputSize_1_56(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureNameInvalidDeviceNode_1_57) {
  getModuleManufactureNameInvalidDeviceNode_1_57(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureNameInvalidHostLatency_1_58) {
  getModuleManufactureNameInvalidHostLatency_1_58(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureNameInvalidDeviceLatency_1_59) {
  getModuleManufactureNameInvalidDeviceLatency_1_59(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, getModuleManufactureNameInvalidOutputBuffer_1_60) {
  getModuleManufactureNameInvalidOutputBuffer_1_60(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieFullBoot, setModulePowerStateRangeInvalidInputBuffer_1_61) {
  setModulePowerStateRangeInvalidInputBuffer_1_61(false /* Multiple Devices */);
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
