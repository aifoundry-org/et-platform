//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef TEST_DEVICE_M_H
#define TEST_DEVICE_M_H

#include "deviceManagement/DeviceManagement.h"
#include "utils.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <dlfcn.h>

using namespace dev;
using namespace device_management;

#define DM_LOG(severity) ET_LOG(DM, severity)
#define DM_DLOG(severity) ET_DLOG(DM, severity)
#define DM_VLOG(severity) ET_VLOG(DM, severity)

#define MAX_DEVICE_NODE (250)
#define INPUT_SIZE_TEST (1)
#define OUTPUT_SIZE_TEST (24)
#define DM_CMD_INVALID (256)
#define INVALID_INPUT_SIZE (0)
#define INVALID_OUTPUT_SIZE (0)

DECLARE_bool(loopback_driver);
DECLARE_string(trace_logfile_txt);
DECLARE_string(trace_logfile_bin);
DECLARE_bool(enable_trace_dump);

void testSerial(device_management::DeviceManagement& dm, uint32_t deviceIdx, uint32_t index, uint32_t timeout,
                int* result);

class TestDevMgmtApiSyncCmds : public ::testing::Test {
protected:
  device_management::getDM_t getInstance();
  void printSpTraceData(const unsigned char*, size_t);
  void extractAndPrintTraceData(void);
  void getModuleManufactureName_1_1(bool singleDevice);
  void getModulePartNumber_1_2(bool singleDevice);
  void getModuleSerialNumber_1_3(bool singleDevice);
  void getASICChipRevision_1_4(bool singleDevice);
  void getModulePCIENumPortsMaxSpeed_1_5(bool singleDevice);
  void getModuleMemorySizeMB_1_6(bool singleDevice);
  void getModuleRevision_1_7(bool singleDevice);
  void getModuleFormFactor_1_8(bool singleDevice);
  void getModuleMemoryVendorPartNumber_1_9(bool singleDevice);
  void getModuleMemoryType_1_10(bool singleDevice);
  void getModulePowerState_1_11(bool singleDevice);
  void setAndGetModuleStaticTDPLevel_1_12(bool singleDevice);
  void setAndGetModuleTemperatureThreshold_1_13(bool singleDevice);
  void getModuleResidencyThrottleState_1_14(bool singleDevice);
  void getModuleUptime_1_15(bool singleDevice);
  void getModulePower_1_16(bool singleDevice);
  void getModuleVoltage_1_17(bool singleDevice);
  void getModuleCurrentTemperature_1_18(bool singleDevice);
  void getModuleMaxTemperature_1_19(bool singleDevice);
  void getModuleMaxMemoryErrors_1_20(bool singleDevice);
  void getModuleMaxDDRBW_1_21(bool singleDevice);
  void getModuleResidencyPowerState_1_22(bool singleDevice);
  void setAndGetDDRECCThresholdCount_1_23(bool singleDevice);
  void setAndGetSRAMECCThresholdCount_1_24(bool singleDevice);
  void setAndGetPCIEECCThresholdCount_1_25(bool singleDevice);
  void getPCIEECCUECCCount_1_26(bool singleDevice);
  void getDDRECCUECCCount_1_27(bool singleDevice);
  void getSRAMECCUECCCount_1_28(bool singleDevice);
  void getDDRBWCounter_1_29(bool singleDevice);
  void setPCIELinkSpeed_1_30(bool singleDevice);
  void setPCIELaneWidth_1_31(bool singleDevice);
  void setPCIERetrainPhy_1_32(bool singleDevice);
  void getASICFrequencies_1_33(bool singleDevice);
  void getDRAMBW_1_34(bool singleDevice);
  void getDRAMCapacityUtilization_1_35(bool singleDevice);
  void getASICPerCoreDatapathUtilization_1_36(bool singleDevice);
  void getASICUtilization_1_37(bool singleDevice);
  void getASICStalls_1_38(bool singleDevice);
  void getASICLatency_1_39(bool singleDevice);
  void getMMErrorCount_1_40(bool singleDevice);
  void getFWBootstatus_1_41(bool singleDevice);
  void getModuleFWRevision_1_42(bool singleDevice);
  void serializeAccessMgmtNode_1_43(bool singleDevice);
  void getDeviceErrorEvents_1_44(bool singleDevice);
  void isUnsupportedService_1_45(bool singleDevice);
  void setSpRootCertificate_1_46(bool singleDevice);
  void setTraceControl_1_47(bool singleDevice);
  void setTraceConfigure_1_48(bool singleDevice, uint32_t event_type, uint32_t filter_level);
  void getTraceBuffer_1_49(bool singleDevice);
  void setModuleActivePowerManagementRange_1_50(bool singleDevice);
  void setModuleSetTemperatureThresholdRange_1_51(bool singleDevice);
  void setModuleStaticTDPLevelRange_1_52(bool singleDevice);
  void setPCIEMaxLinkSpeedRange_1_53(bool singleDevice);
  void setPCIELaneWidthRange_1_54(bool singleDevice);
  void setModuleActivePowerManagementRangeInvalidInputSize_1_55(bool singleDevice);
  void getModuleManufactureNameInvalidOutputSize_1_56(bool singleDevice);
  void getModuleManufactureNameInvalidDeviceNode_1_57(bool singleDevice);
  void getModuleManufactureNameInvalidHostLatency_1_58(bool singleDevice);
  void getModuleManufactureNameInvalidDeviceLatency_1_59(bool singleDevice);
  void getModuleManufactureNameInvalidOutputBuffer_1_60(bool singleDevice);
  void setModuleActivePowerManagementRangeInvalidInputBuffer_1_61(bool singleDevice);
  void setModuleActivePowerManagement_1_62(bool singleDevice);
  void updateFirmwareImage_1_63(bool singleDevice);
  void setPCIELinkSpeedToInvalidLinkSpeed_1_64(bool singleDevice);
  void setPCIELaneWidthToInvalidLaneWidth_1_65(bool singleDevice);
  void testInvalidOutputSize(int32_t dmCmdType, bool singleDevice);
  void testInvalidDeviceNode(int32_t dmCmdType, bool singleDevice);
  void testInvalidHostLatency(int32_t dmCmdType, bool singleDevice);
  void testInvalidDeviceLatency(int32_t dmCmdType, bool singleDevice);
  void testInvalidOutputBuffer(int32_t dmCmdType, bool singleDevice);
  void getASICFrequenciesInvalidOutputSize_1_66(bool singleDevice);
  void getASICFrequenciesInvalidDeviceNode_1_67(bool singleDevice);
  void getASICFrequenciesInvalidHostLatency_1_68(bool singleDevice);
  void getASICFrequenciesInvalidDeviceLatency_1_69(bool singleDevice);
  void getASICFrequenciesInvalidOutputBuffer_1_70(bool singleDevice);
  void getDRAMBandwidthInvalidOutputSize_1_71(bool singleDevice);
  void getDRAMBandwidthInvalidDeviceNode_1_72(bool singleDevice);
  void getDRAMBandwidthInvalidHostLatency_1_73(bool singleDevice);
  void getDRAMBandwidthInvalidDeviceLatency_1_74(bool singleDevice);
  void getDRAMBandwidthInvalidOutputBuffer_1_75(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidOutputSize_1_76(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidDeviceNode_1_77(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidHostLatency_1_78(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidDeviceLatency_1_79(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidOutputBuffer_1_80(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidOutputSize_1_81(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidDeviceNode_1_82(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidHostLatency_1_83(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidDeviceLatency_1_84(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidOutputBuffer_1_85(bool singleDevice);
  void getASICUtilizationInvalidOutputSize_1_86(bool singleDevice);
  void getASICUtilizationInvalidDeviceNode_1_87(bool singleDevice);
  void getASICUtilizationInvalidHostLatency_1_88(bool singleDevice);
  void getASICUtilizationInvalidDeviceLatency_1_89(bool singleDevice);
  void getASICUtilizationInvalidOutputBuffer_1_90(bool singleDevice);
  void getASICStallsInvalidOutputSize_1_91(bool singleDevice);
  void getASICStallsInvalidDeviceNode_1_92(bool singleDevice);
  void getASICStallsInvalidHostLatency_1_93(bool singleDevice);
  void getASICStallsInvalidDeviceLatency_1_94(bool singleDevice);
  void getASICStallsInvalidOutputBuffer_1_95(bool singleDevice);
  void getASICLatencyInvalidOutputSize_1_96(bool singleDevice);
  void getASICLatencyInvalidDeviceNode_1_97(bool singleDevice);
  void getASICLatencyInvalidHostLatency_1_98(bool singleDevice);
  void getASICLatencyInvalidDeviceLatency_1_99(bool singleDevice);
  void getASICLatencyInvalidOutputBuffer_1_100(bool singleDevice);
  void testInvalidCmdCode_1_101(bool singleDevice);
  void testInvalidInputBuffer(int32_t dmCmdType, bool singleDevice);
  void testInvalidInputSize(int32_t dmCmdType, bool singleDevice);
  void setDDRECCCountInvalidInputBuffer_1_102(bool singleDevice);
  void setDDRECCCountInvalidInputSize_1_103(bool singleDevice);
  void setDDRECCCountInvalidOutputSize_1_104(bool singleDevice);
  void setDDRECCCountInvalidDeviceNode_1_105(bool singleDevice);
  void setDDRECCCountInvalidHostLatency_1_106(bool singleDevice);
  void setDDRECCCountInvalidDeviceLatency_1_107(bool singleDevice);
  void setDDRECCCountInvalidOutputBuffer_1_108(bool singleDevice);
  void setPCIEECCCountInvalidInputBuffer_1_109(bool singleDevice);
  void setPCIEECCountInvalidInputSize_1_110(bool singleDevice);
  void setPCIEECCountInvalidOutputSize_1_111(bool singleDevice);
  void setPCIEECCountInvalidDeviceNode_1_112(bool singleDevice);
  void setPCIEECCountInvalidHostLatency_1_113(bool singleDevice);
  void setPCIEECCountInvalidDeviceLatency_1_114(bool singleDevice);
  void setPCIEECCountInvalidOutputBuffer_1_115(bool singleDevice);
  void setSRAMECCCountInvalidInputBuffer_1_116(bool singleDevice);
  void setSRAMECCCountInvalidInputSize_1_117(bool singleDevice);
  void setSRAMECCountInvalidOutputSize_1_118(bool singleDevice);
  void setSRAMECCountInvalidDeviceNode_1_119(bool singleDevice);
  void setSRAMECCountInvalidHostLatency_1_120(bool singleDevice);
  void setSRAMECCountInvalidDeviceLatency_1_121(bool singleDevice);
  void setSRAMECCountInvalidOutputBuffer_1_122(bool singleDevice);
  void getHistoricalExtremeWithInvalidDeviceNode_1_123(bool singleDevice);
  void getHistoricalExtremeWithInvalidHostLatency_1_124(bool singleDevice);
  void getHistoricalExtremeWithInvalidDeviceLatency_1_125(bool singleDevice);
  void getHistoricalExtremeWithInvalidOutputBuffer_1_126(bool singleDevice);
  void getHistoricalExtremeWithInvalidOutputSize_1_127(bool singleDevice);
  void setThrottlePowerStatus_1_128(bool singleDevice);
  void* handle_ = nullptr;
  std::unique_ptr<IDeviceLayer> devLayer_;
};

#endif // TEST_DEVICE_M_H
