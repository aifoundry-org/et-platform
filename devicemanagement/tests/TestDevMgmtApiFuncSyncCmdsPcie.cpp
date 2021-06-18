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

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleManufactureName_1_1) {
  getModuleManufactureName_1_1();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePartNumber_1_2) {
  getModulePartNumber_1_2();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleSerialNumber_1_3) {
  getModuleSerialNumber_1_3();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICChipRevision_1_4) {
  getASICChipRevision_1_4();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePCIENumPortsMaxSpeed_1_5) {
  getModulePCIENumPortsMaxSpeed_1_5();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMemorySizeMB_1_6) {
  getModuleMemorySizeMB_1_6();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleRevision_1_7) {
  getModuleRevision_1_7();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleFormFactor_1_8) {
  getModuleFormFactor_1_8();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMemoryVendorPartNumber_1_9) {
  getModuleMemoryVendorPartNumber_1_9();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMemoryType_1_10) {
  getModuleMemoryType_1_10();
}

// Thermal and Power
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetModulePowerState_1_11) {
  setAndGetModulePowerState_1_11();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetModuleStaticTDPLevel_1_12) {
  setAndGetModuleStaticTDPLevel_1_12();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetModuleTemperatureThreshold_1_13) {
  setAndGetModuleTemperatureThreshold_1_13();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleResidencyThrottleState_1_14) {
  getModuleResidencyThrottleState_1_14();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleUptime_1_15) {
  getModuleUptime_1_15();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModulePower_1_16) {
  getModulePower_1_16();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleVoltage_1_17) {
  getModuleVoltage_1_17();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleCurrentTemperature_1_18) {
  getModuleCurrentTemperature_1_18();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxTemperature_1_19) {
  getModuleMaxTemperature_1_19();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxMemoryErrors_1_20) {
  getModuleMaxMemoryErrors_1_20();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxDDRBW_1_21) {
  getModuleMaxDDRBW_1_21();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleMaxThrottleTime_1_22) {
  getModuleMaxThrottleTime_1_22();
}

// Error Control
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetDDRECCThresholdCount_1_23) {
  setAndGetDDRECCThresholdCount_1_23();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetSRAMECCThresholdCount_1_24) {
  setAndGetSRAMECCThresholdCount_1_24();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setAndGetPCIEECCThresholdCount_1_25) {
  setAndGetPCIEECCThresholdCount_1_25();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getPCIEECCUECCCount_1_26) {
  getPCIEECCUECCCount_1_26();
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
  getDDRECCUECCCount_1_27();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getSRAMECCUECCCount_1_28) {
  getSRAMECCUECCCount_1_28();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDDRBWCounter_1_29) {
  getDDRBWCounter_1_29();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELinkSpeed_1_30) {
  setPCIELinkSpeed_1_30();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIELaneWidth_1_31) {
  setPCIELaneWidth_1_31();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setPCIERetrainPhy_1_32) {
  setPCIERetrainPhy_1_32();
}

// Performance
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICFrequencies_1_33) {
  getASICFrequencies_1_33();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDRAMBW_1_34) {
  getDRAMBW_1_34();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDRAMCapacityUtilization_1_35) {
  getDRAMCapacityUtilization_1_35();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICPerCoreDatapathUtilization_1_36) {
  getASICPerCoreDatapathUtilization_1_36();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICUtilization_1_37) {
  getASICUtilization_1_37();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICStalls_1_38) {
  getASICStalls_1_38();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getASICLatency_1_39) {
  getASICLatency_1_39();
}

#ifdef TARGET_PCIE
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getFWBootstatus_1_41) {
  getFWBootstatus_1_41();
}

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getModuleFWRevision_1_42) {
  getModuleFWRevision_1_42();
}
#endif

/*
// Test serial access
// SW-6404 Test failing in Zebu
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, serializeAccessMgmtNode_1_43) {
  serializeAccessMgmtNode_1_43();
}
*/
#if 0 // TODO: Enable with the fix for https://esperantotech.atlassian.net/browse/SW-7920
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44();
}
#endif

TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, isUnsupportedService_1_45) {
  isUnsupportedService_1_45();
}

/*TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, setSpRootCertificate_1_46) {
  setSpRootCertificate_1_46();
}*/

// retrieve MM FW error counts. This test should be run last so that we are
// able to capture any errors in the previous test runs
TEST_F(TestDevMgmtApiFuncSyncCmdsPcie, getMMErrorCount_1_40) {
  getMMErrorCount_1_40();
}

// TODO: conditional to be removed with https://esperantotech.atlassian.net/browse/SW-6044
#ifdef TARGET_PCIE
/*
// TODO : Enable Firmware update test case
// copy /projects/esperanto/img_v002/ to your sw-platform directory and change IMG_USER below
#define IMG_PRE "/projects/esperanto/"
#define IMG_USER "mpowell"
#define IMG_POST "/sw-platform/img_v002"
#define SP_CRT_512_V002 IMG_PRE IMG_USER IMG_POST "/hash.txt"
#define IMG_V002 IMG_PRE IMG_USER IMG_POST "/flash_16Mbit.img"

// Test DM_CMD_SET_FIRMWARE_UPDATE
TEST_F(TestDevMgmtApiSyncCmds, test_DM_CMD_SET_FIRMWARE_UPDATE) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)(devLayer_.get());

  // DM_CMD_SET_FIRMWARE_UPDATE : Device returns response of type device_mgmt_default_rsp_t.
  // Payload in response is of type uint32_t
  const uint32_t output_size = sizeof(uint32_t);

  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE, IMG_V002, 1, output_buff,
output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "0", output_size);
  printf("expected: %.*s\n", output_size, expected);

  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// TODO : Enable Firmware update test case
// Test firmware_update
TEST_F(TestDevMgmtApiSyncCmds, test_firmware_update) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)(devLayer_.get());

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  // Step 1: GET_MODULE_FIRMWARE_REVISIONS
  const uint32_t rev1_output_size = sizeof(device_mgmt_api::firmware_version_t);
  char rev1_output_buff[rev1_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
rev1_output_buff, rev1_output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

printf("rev1_output_buff: %.*s\n", rev1_output_size, rev1_output_buff);

  device_mgmt_api::firmware_version_t *firmware_versions = (device_mgmt_api::firmware_version_t *)rev1_output_buff;

  ASSERT_EQ(firmware_versions->bl1_v, 0010);
  ASSERT_EQ(firmware_versions->bl2_v, 0010);
  ASSERT_EQ(firmware_versions->mm_v, 0010);
  ASSERT_EQ(firmware_versions->wm_v, 0010);
  ASSERT_EQ(firmware_versions->machm_v, 0010);

  // Step 2: SET_SP_BOOT_ROOT_CERT
  //Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
  const uint32_t crt_output_size = sizeof(uint32_t);
  char crt_output_buff[crt_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1 ,
crt_output_buff, crt_output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

  printf("crt_output_buff: %.*s\n", crt_output_size,crt_output_buff);

  char crt_expected[crt_output_size] = {0};
  strncpy(crt_expected, "0", crt_output_size);
  printf("crt_expected: %.*s\n", crt_output_size, crt_expected);
  ASSERT_EQ(strncmp(crt_output_buff, crt_expected, crt_output_size), 0);

  // Step 3: SET_FIRMWARE_UPDATE
  //Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
  const uint32_t fw_output_size = sizeof(uint32_t);
  char fw_output_buff[fw_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE, IMG_V002, 1,
fw_output_buff, fw_output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

  printf("fw_output_buff: %.*s\n", fw_output_size, fw_output_buff);

  char fw_expected[fw_output_size] = {0};
  strncpy(fw_expected, "0", fw_output_size);
  printf("fw_expected: %.*s\n", fw_output_size, fw_expected);
  ASSERT_EQ(strncmp(fw_output_buff, fw_expected, fw_output_size), 0);
}

// Test verify_firmware_update (Manual continutation after SP resets)
TEST_F(TestDevMgmtApiSyncCmds, test_verify_firmware_update) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)(devLayer_.get());

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  // Step 4: GET_FIRMWARE_BOOT_STATUS
  //Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
  const uint32_t bt_output_size = sizeof(uint32_t);
  char bt_output_buff[bt_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS, nullptr, 0,
bt_output_buff, bt_output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

  printf("bt_output_buff: %.*s\n", bt_output_size, bt_output_buff);

  char bt_expected[bt_output_size] = {0};
  strncpy(bt_expected, "0", bt_output_size);
  printf("bt_expected: %.*s\n", bt_output_size, bt_expected);
  ASSERT_EQ(strncmp(bt_output_buff, bt_expected, bt_output_size), 0);

  // Step 5: GET_MODULE_FIRMWARE_REVISIONS
  const uint32_t rev2_output_size = sizeof(device_mgmt_api::firmware_version_t);
  char rev2_output_buff[rev2_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
rev2_output_buff, rev2_output_size, hst_latency.get(), dev_latency.get(), 2000), device_mgmt_api::DM_STATUS_SUCCESS);

  printf("rev2_output_buff: %.*s\n", rev2_output_size, rev2_output_buff);

  device_mgmt_api::firmware_version_t *firmware_versions = (device_mgmt_api::firmware_version_t *)rev2_output_buff;

  ASSERT_EQ(firmware_versions->bl1_v, 0020);
  ASSERT_EQ(firmware_versions->bl2_v, 0020);
  ASSERT_EQ(firmware_versions->mm_v, 0020);
  ASSERT_EQ(firmware_versions->wm_v, 0020);
  ASSERT_EQ(firmware_versions->machm_v, 0020);
}
*/
#endif

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
