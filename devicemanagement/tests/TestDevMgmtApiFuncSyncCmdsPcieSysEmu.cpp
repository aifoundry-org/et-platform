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
  getModuleManufactureName_1_1();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePartNumber_1_2) {
  getModulePartNumber_1_2();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleSerialNumber_1_3) {
  getModuleSerialNumber_1_3();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemorySizeMB_1_6) {
  getModuleMemorySizeMB_1_6();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleRevision_1_7) {
  getModuleRevision_1_7();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleFormFactor_1_8) {
  getModuleFormFactor_1_8();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10();
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModulePowerState_1_11) {
  setAndGetModulePowerState_1_11();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleUptime_1_15) {
  getModuleUptime_1_15();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMaxMemoryErrors_1_20) {
  getModuleMaxMemoryErrors_1_20();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleMaxThrottleTime_1_22) {
  getModuleMaxThrottleTime_1_22();
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setAndGetPCIEECCThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDDRECCUECCCount_1_27) {
  getDDRECCUECCCount_1_27();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32();
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICFrequencies_1_33) {
  getASICFrequencies_1_33();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICUtilization_1_37) {
  getASICUtilization_1_37();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICStalls_1_38) {
  getASICStalls_1_38();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getASICLatency_1_39) {
  getASICLatency_1_39();
}

#ifdef TARGET_PCIE
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getFWBootstatus_1_41) {
  getFWBootstatus_1_41();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getModuleFWRevision_1_42) {
  getModuleFWRevision_1_42();
}
#endif

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, isUnsupportedService_1_45) {
  isUnsupportedService_1_45();
}

/*TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, setSpRootCertificate_1_46) {
  setSpRootCertificate_1_46();
}*/

// retrieve MM FW error counts. This test should be run last so that we are
// able to capture any errors in the previous test runs
TEST_F(TestDevMgmtApiFuncSyncCmdsPcieSysEmu, getMMErrorCount_1_40) {
  getMMErrorCount_1_40();
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
