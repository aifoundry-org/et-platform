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
#include "DeviceSysEmu.h"
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

void testSerial(device_management::DeviceManagement &dm, uint32_t index, uint32_t timeout, int *result);

class TestDevMgmtApiSyncCmds : public ::testing::Test {
protected:
  device_management::getDM_t getInstance();

  void getModuleManufactureName_1_1();
  void getModulePartNumber_1_2();
  void getModuleSerialNumber_1_3();
  void getASICChipRevision_1_4();
  void getModulePCIENumPortsMaxSpeed_1_5();
  void getModuleMemorySizeMB_1_6();
  void getModuleRevision_1_7();
  void getModuleFormFactor_1_8();
  void getModuleMemoryVendorPartNumber_1_9();
  void getModuleMemoryType_1_10();
  void setAndGetModulePowerState_1_11();
  void setAndGetModuleStaticTDPLevel_1_12();
  void setAndGetModuleTemperatureThreshold_1_13();
  void getModuleResidencyThrottleState_1_14();
  void getModuleUptime_1_15();
  void getModulePower_1_16();
  void getModuleVoltage_1_17();
  void getModuleCurrentTemperature_1_18();
  void getModuleMaxTemperature_1_19();
  void getModuleMaxMemoryErrors_1_20();
  void getModuleMaxDDRBW_1_21();
  void getModuleMaxThrottleTime_1_22();
  void setAndGetDDRECCThresholdCount_1_23();
  void setAndGetSRAMECCThresholdCount_1_24();
  void setAndGetPCIEECCThresholdCount_1_25();
  void getPCIEECCUECCCount_1_26();
  void getDDRECCUECCCount_1_27();
  void getSRAMECCUECCCount_1_28();
  void getDDRBWCounter_1_29();
  void setPCIELinkSpeed_1_30();
  void setPCIELaneWidth_1_31();
  void setPCIERetrainPhy_1_32();
  void getASICFrequencies_1_33();
  void getDRAMBW_1_34();
  void getDRAMCapacityUtilization_1_35();
  void getASICPerCoreDatapathUtilization_1_36();
  void getASICUtilization_1_37();
  void getASICStalls_1_38();
  void getASICLatency_1_39();

  void getMMErrorCount_1_40();
  void getFWBootstatus_1_41();
  void getModuleFWRevision_1_42();

  void serializeAccessMgmtNode_1_43();
  void getDeviceErrorEvents_1_44();
  void isUnsupportedService_1_45();

  void *handle_ = nullptr;
  std::unique_ptr<IDeviceLayer> devLayer_;
};

#endif // TEST_DEVICE_M_H

