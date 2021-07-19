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
  void* handle_ = nullptr;
  std::unique_ptr<IDeviceLayer> devLayer_;
};

#endif // TEST_DEVICE_M_H
