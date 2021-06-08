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
#include <device-layer/Autogen.h>
#include <dlfcn.h>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;
using namespace device_management;

namespace {
  constexpr int kIDevice = 0;
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} //namespace

class TestDevMgmtApiFuncSyncCmdsSysEmu : public TestDevMgmtApiSyncCmds {
public:
  void SetUp() override {
    handle_ = nullptr;
    handle_ = dlopen("libDM.so", RTLD_LAZY);

    emu::SysEmuOptions sysEmuOptions;

    sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
    sysEmuOptions.spBL2ElfPath = BL2_ELF;
    sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
    sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF;
    sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
    sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
    sysEmuOptions.runDir = fs::current_path();
    sysEmuOptions.maxCycles = kSysEmuMaxCycles;
    sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
    sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
    sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
    sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
    sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
    sysEmuOptions.startGdb = false;

    // Launch Sysemu through IDevice Abstraction
    devLayer_ = IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);
  }

  void TearDown() override { if (handle_ != nullptr) { dlclose(handle_); } }
};


TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleManufactureName_1_1) {
  getModuleManufactureName_1_1();
}


TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModulePartNumber_1_2) {
  getModulePartNumber_1_2();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleSerialNumber_1_3) {
  getModuleSerialNumber_1_3();
}


TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMemorySizeMB_1_6) {
  getModuleMemorySizeMB_1_6();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleRevision_1_7) {
  getModuleRevision_1_7();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleFormFactor_1_8) {
  getModuleFormFactor_1_8();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10();
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetModulePowerState_1_11) {
  setAndGetModulePowerState_1_11();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleUptime_1_15) {
  getModuleUptime_1_15();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMaxMemoryErrors_1_20) {
  getModuleMaxMemoryErrors_1_20();
}
/* TODO Require SysEMU modelling fix
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMaxDDRBW_1_21) {
  getModuleMaxDDRBW_1_21();
}
*/

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleMaxThrottleTime_1_22) {
  getModuleMaxThrottleTime_1_22();
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setAndGetPCIEECThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getDDRECCUECCCount_1_27) {
  getDDRECCUECCCount_1_27();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28();
}

/* TODO: Remove this test
   Note: Device(BL2 DM TASK) initializes the DDR BW Counter. DM task is dependent on PMIC as well
   This test should be run only on Zebu.
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getModuleDDRBWCounter_1_29) {
  getModuleDDRBWCounter_1_29();
}
*/

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32();
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getASICFrequencies_1_33) {
  getASICFrequencies_1_33();
}

/* TODO Require SysEMU modelling fix
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getDRAMBW_1_34) {
  getDRAMBW_1_34();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getDRAMCapacityUtilization_1_35) {
  getDRAMCapacityUtilization_1_35();
}
*/

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getASICUtilization_1_37) {
  getASICUtilization_1_37();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getASICStalls_1_38) {
  getASICStalls_1_38();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getASICLatency_1_39) {
  getASICLatency_1_39();
}

// Test serial access
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, serializeAccessMgmtNode_1_43) {
  serializeAccessMgmtNode_1_43();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, isUnsupportedService_1_45) {
  isUnsupportedService_1_45();
}

// retrieve MM FW error counts. This test should be run last so that we are
// able to capture any errors in the previous test runs
TEST_F(TestDevMgmtApiFuncSyncCmdsSysEmu, getMMErrorCount_1_40) {
  getMMErrorCount_1_40();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "2");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
