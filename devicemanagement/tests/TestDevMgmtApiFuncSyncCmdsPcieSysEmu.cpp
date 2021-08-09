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

class TestDevMgmtApiFuncSyncCmdsPcieSysEmu : public TestDevMgmtApiSyncCmds {
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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureName_1_1) {
  getModuleManufactureName_1_1(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePartNumber_1_2) {
  getModulePartNumber_1_2(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleSerialNumber_1_3) {
  getModuleSerialNumber_1_3(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemorySizeMB_1_6) {
  getModuleMemorySizeMB_1_6(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleRevision_1_7) {
  getModuleRevision_1_7(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleFormFactor_1_8) {
  getModuleFormFactor_1_8(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10(false /* single device */);
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModulePowerState_1_11) {
  setAndGetModulePowerState_1_11(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleUptime_1_15) {
  getModuleUptime_1_15(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMaxMemoryErrors_1_20) {
  getModuleMaxMemoryErrors_1_20(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleResidencyPowerState_1_22) {
  getModuleResidencyPowerState_1_22(false /* single device */);
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetPCIEECCThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDDRECCUECCCount_1_27) {
  getDDRECCUECCCount_1_27(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32(false /* single device */);
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequencies_1_33) {
  getASICFrequencies_1_33(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilization_1_37) {
  getASICUtilization_1_37(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStalls_1_38) {
  getASICStalls_1_38(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatency_1_39) {
  getASICLatency_1_39(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, serializeAccessMgmtNode_1_43) {
  serializeAccessMgmtNode_1_43(false);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, isUnsupportedService_1_45) {
  isUnsupportedService_1_45(false /* single device */);
}

/*TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSpRootCertificate_1_46) {
  setSpRootCertificate_1_46(false);
}*/

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setTraceControl_1_47) {
  setTraceControl_1_47(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setTraceConfigure_1_48) {
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getTraceBuffer_1_49) {
  setTraceControl_1_47(false /* Multiple devices */);
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG);
  setAndGetModuleStaticTDPLevel_1_12(false /* Multiple devices */);
  getTraceBuffer_1_49(false /* Multiple devices */);

  /* Restore the logging level back */
  setTraceConfigure_1_48(false /* Multiple devices */, device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                         device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModulePowerStateRange_1_50) {
  setModulePowerStateRange_1_50(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleSetTemperatureThresholdRange_1_51) {
  setModuleSetTemperatureThresholdRange_1_51(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleStaticTDPLevelRange_1_52) {
  setModuleStaticTDPLevelRange_1_52(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModulePowerStateRangeInvalidInputSize_1_55) {
  setModulePowerStateRangeInvalidInputSize_1_55(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureNameInvalidOutputSize_1_56) {
  getModuleManufactureNameInvalidOutputSize_1_56(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureNameInvalidDeviceNode_1_57) {
  getModuleManufactureNameInvalidDeviceNode_1_57(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureNameInvalidHostLatency_1_58) {
  getModuleManufactureNameInvalidHostLatency_1_58(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureNameInvalidDeviceLatency_1_59) {
  getModuleManufactureNameInvalidDeviceLatency_1_59(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleManufactureNameInvalidOutputBuffer_1_60) {
  getModuleManufactureNameInvalidOutputBuffer_1_60(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModulePowerStateRangeInvalidInputBuffer_1_61) {
  setModulePowerStateRangeInvalidInputBuffer_1_61(false /* Multiple Devices */);
}

// retrieve MM FW error counts. This test should be run last so that we are
// able to capture any errors in the previous test runs
// Note after fix from: SW-8409, counters needs to adjusted to handle expected failures
// TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getMMErrorCount_1_40) {
//  getMMErrorCount_1_40(false /* single device */);
//}

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
