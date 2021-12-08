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

class TestDevMgmtApiFuncSyncCmdsPcieSysEmu : public TestDevMgmtApiSyncCmds {
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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9(false /* single device */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10(false /* single device */);
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePowerState_1_11) {
  getModulePowerState_1_11(false /* single device */);
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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleActivePowerManagementRange_1_50) {
  setModuleActivePowerManagementRange_1_50(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleSetTemperatureThresholdRange_1_51) {
  setModuleSetTemperatureThresholdRange_1_51(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleStaticTDPLevelRange_1_52) {
  setModuleStaticTDPLevelRange_1_52(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleActivePowerManagementRangeInvalidInputSize_1_55) {
  setModuleActivePowerManagementRangeInvalidInputSize_1_55(false /* Multiple Devices */);
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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setModuleActivePowerManagementRangeInvalidInputBuffer_1_61) {
  setModuleActivePowerManagementRangeInvalidInputBuffer_1_61(false /* Multiple Devices */);
}

// Pending SysEMU pointer update. Stuck behind a modelling bug
// TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, updateFirmwareImage_1_63) {
//  updateFirmwareImage_1_63(false);
//}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELinkSpeedToInvalidLinkSpeed_1_64) {
  setPCIELinkSpeedToInvalidLinkSpeed_1_64(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELaneWidthToInvalidLaneWidth_1_65) {
  setPCIELaneWidthToInvalidLaneWidth_1_65(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequenciesInvalidOutputSize_1_66) {
  getASICFrequenciesInvalidOutputSize_1_66(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequenciesInvalidDeviceNode_1_67) {
  getASICFrequenciesInvalidDeviceNode_1_67(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequenciesInvalidHostLatency_1_68) {
  getASICFrequenciesInvalidHostLatency_1_68(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequenciesInvalidDeviceLatency_1_69) {
  getASICFrequenciesInvalidDeviceLatency_1_69(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequenciesInvalidOutputBuffer_1_70) {
  getASICFrequenciesInvalidOutputBuffer_1_70(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMBandwidthInvalidOutputSize_1_71) {
  getDRAMBandwidthInvalidOutputSize_1_71(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMBandwidthInvalidDeviceNode_1_72) {
  getDRAMBandwidthInvalidDeviceNode_1_72(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMBandwidthInvalidHostLatency_1_73) {
  getDRAMBandwidthInvalidHostLatency_1_73(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMBandwidthInvalidDeviceLatency_1_74) {
  getDRAMBandwidthInvalidDeviceLatency_1_74(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMBandwidthInvalidOutputBuffer_1_75) {
  getDRAMBandwidthInvalidOutputBuffer_1_75(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMCapacityUtilizationInvalidOutputSize_1_76) {
  getDRAMCapacityUtilizationInvalidOutputSize_1_76(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMCapacityUtilizationInvalidDeviceNode_1_77) {
  getDRAMCapacityUtilizationInvalidDeviceNode_1_77(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMCapacityUtilizationInvalidHostLatency_1_78) {
  getDRAMCapacityUtilizationInvalidHostLatency_1_78(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMCapacityUtilizationInvalidDeviceLatency_1_79) {
  getDRAMCapacityUtilizationInvalidDeviceLatency_1_79(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDRAMCapacityUtilizationInvalidOutputBuffer_1_80) {
  getDRAMCapacityUtilizationInvalidOutputBuffer_1_80(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilizationInvalidOutputSize_1_81) {
  getASICPerCoreDatapathUtilizationInvalidOutputSize_1_81(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilizationInvalidDeviceNode_1_82) {
  getASICPerCoreDatapathUtilizationInvalidDeviceNode_1_82(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilizationInvalidHostLatency_1_83) {
  getASICPerCoreDatapathUtilizationInvalidHostLatency_1_83(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilizationInvalidDeviceLatency_1_84) {
  getASICPerCoreDatapathUtilizationInvalidDeviceLatency_1_84(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilizationInvalidOutputBuffer_1_85) {
  getASICPerCoreDatapathUtilizationInvalidOutputBuffer_1_85(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilizationInvalidOutputSize_1_86) {
  getASICUtilizationInvalidOutputSize_1_86(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilizationInvalidDeviceNode_1_87) {
  getASICUtilizationInvalidDeviceNode_1_87(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilizationInvalidHostLatency_1_88) {
  getASICUtilizationInvalidHostLatency_1_88(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilizationInvalidDeviceLatency_1_89) {
  getASICUtilizationInvalidDeviceLatency_1_89(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilizationInvalidOutputBuffer_1_90) {
  getASICUtilizationInvalidOutputBuffer_1_90(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStallsInvalidOutputSize_1_91) {
  getASICStallsInvalidOutputSize_1_91(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStallsInvalidDeviceNode_1_92) {
  getASICStallsInvalidDeviceNode_1_92(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStallsInvalidHostLatency_1_93) {
  getASICStallsInvalidHostLatency_1_93(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStallsInvalidDeviceLatency_1_94) {
  getASICStallsInvalidDeviceLatency_1_94(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStallsInvalidOutputBuffer_1_95) {
  getASICStallsInvalidOutputBuffer_1_95(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatencyInvalidOutputSize_1_96) {
  getASICLatencyInvalidOutputSize_1_96(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatencyInvalidDeviceNode_1_97) {
  getASICLatencyInvalidDeviceNode_1_97(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatencyInvalidHostLatency_1_98) {
  getASICLatencyInvalidHostLatency_1_98(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatencyInvalidDeviceLatency_1_99) {
  getASICLatencyInvalidDeviceLatency_1_99(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatencyInvalidOutputBuffer_1_100) {
  getASICLatencyInvalidOutputBuffer_1_100(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, testInvalidCmdCode_1_101) {
  testInvalidCmdCode_1_101(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidInputBuffer_1_102) {
  setDDRECCCountInvalidInputBuffer_1_102(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidInputSize_1_103) {
  setDDRECCCountInvalidInputSize_1_103(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidOutputSize_1_104) {
  setDDRECCCountInvalidOutputSize_1_104(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidDeviceNode_1_105) {
  setDDRECCCountInvalidDeviceNode_1_105(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidHostLatency_1_106) {
  setDDRECCCountInvalidHostLatency_1_106(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidDeviceLatency_1_107) {
  setDDRECCCountInvalidDeviceLatency_1_107(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setDDRECCCountInvalidOutputBuffer_1_108) {
  setDDRECCCountInvalidOutputBuffer_1_108(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCCountInvalidInputBuffer_1_109) {
  setPCIEECCCountInvalidInputBuffer_1_109(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidInputSize_1_110) {
  setPCIEECCountInvalidInputSize_1_110(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidOutputSize_1_111) {
  setPCIEECCountInvalidOutputSize_1_111(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidDeviceNode_1_112) {
  setPCIEECCountInvalidDeviceNode_1_112(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidHostLatency_1_113) {
  setPCIEECCountInvalidHostLatency_1_113(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidDeviceLatency_1_114) {
  setPCIEECCountInvalidDeviceLatency_1_114(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIEECCountInvalidOutputBuffer_1_115) {
  setPCIEECCountInvalidOutputBuffer_1_115(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCCountInvalidInputBuffer_1_116) {
  setSRAMECCCountInvalidInputBuffer_1_116(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCCountInvalidInputSize_1_117) {
  setSRAMECCCountInvalidInputSize_1_117(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCountInvalidOutputSize_1_118) {
  setSRAMECCountInvalidOutputSize_1_118(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCountInvalidDeviceNode_1_119) {
  setSRAMECCountInvalidDeviceNode_1_119(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCountInvalidHostLatency_1_120) {
  setSRAMECCountInvalidHostLatency_1_120(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCountInvalidDeviceLatency_1_121) {
  setSRAMECCountInvalidDeviceLatency_1_121(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSRAMECCountInvalidOutputBuffer_1_122) {
  setSRAMECCountInvalidOutputBuffer_1_122(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getHistoricalExtremeWithInvalidDeviceNode_1_123) {
  getHistoricalExtremeWithInvalidDeviceNode_1_123(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getHistoricalExtremeWithInvalidHostLatency_1_124) {
  getHistoricalExtremeWithInvalidHostLatency_1_124(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getHistoricalExtremeWithInvalidDeviceLatency_1_125) {
  getHistoricalExtremeWithInvalidDeviceLatency_1_125(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getHistoricalExtremeWithInvalidOutputBuffer_1_126) {
  getHistoricalExtremeWithInvalidOutputBuffer_1_126(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getHistoricalExtremeWithInvalidOutputSize_1_127) {
  getHistoricalExtremeWithInvalidOutputSize_1_127(false /* Multiple Devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setThrottlePowerStatus_1_128) {
  setThrottlePowerStatus_1_128(true /* Single Devices */);
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
  if (FLAGS_loopback_driver) {
    // Loopback driver does not support trace
    FLAGS_enable_trace_dump = false;
  }
  return RUN_ALL_TESTS();
}
