//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/DeviceManagement/DeviceManagement.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstring>
#include <errno.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <dlfcn.h>

using namespace et_runtime;
using namespace device_management;
using namespace std;

class DMTestModule : public ::testing::Test {
public:
  void SetUp() override { handle_ = dlopen("libDM.so", RTLD_LAZY); }
  void TearDown() override { dlclose(handle_); }

  getDM_t getInstance() {
    const char *error;

    if (handle_) {
      getDM_t getDM = reinterpret_cast<getDM_t>(dlsym(handle_, "getInstance"));

      if (!(error = dlerror())) {
        return getDM;
      }
    }
    return (getDM_t)0;
  }

  void *handle_;
};

/*
// Test SP RESEST
TEST_F(DMTestModule, test_SP_RESET) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::RESET_ETSOC, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "0", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/

// Test GET_MODULE_MANUFACTURE_NAME
TEST_F(DMTestModule, test_GET_MODULE_MANUFACTURE_NAME) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MANUFACTURE_NAME, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "Esperant", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_PART_NUMBER
TEST_F(DMTestModule, test_GET_MODULE_PART_NUMBER) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_PART_NUMBER, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "ETPART01", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_SERIAL_NUMBER
TEST_F(DMTestModule, test_GET_MODULE_SERIAL_NUMBER) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_SERIAL_NUMBER, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "ETSERNO1", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

/*
// TODO: Fixme SW-5240: https://esperantotech.atlassian.net/browse/SW-5240
// Test GET_ASIC_CHIP_REVISION
TEST_F(DMTestModule, test_GET_ASIC_CHIP_REVISION) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_ASIC_CHIP_REVISION, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "160", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/
/* Note : This test should be run in full boot mode only.
// Test GET_MODULE_FIRMWARE_REVISIONS
TEST_F(DMTestModule, test_GET_MODULE_FIRMWARE_REVISIONS) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 20;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "00100010001000100010", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/

/*
// Commented out for now due to SW-5240: https://esperantotech.atlassian.net/browse/SW-5240
// Test GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED
TEST_F(DMTestModule, test_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "8", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/
// Test GET_MODULE_MEMORY_SIZE_MB
TEST_F(DMTestModule, test_GET_MODULE_MEMORY_SIZE_MB) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_SIZE_MB, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "16", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_REVISION
TEST_F(DMTestModule, test_GET_MODULE_REVISION) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_REVISION, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "1", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_FORM_FACTOR
TEST_F(DMTestModule, test_GET_MODULE_FORM_FACTOR) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_FORM_FACTOR, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "Dual_M2", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_MEMORY_VENDOR_PART_NUMBER
TEST_F(DMTestModule, test_GET_MODULE_MEMORY_VENDOR_PART_NUMBER) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_VENDOR_PART_NUMBER, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "Micron", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

// Test GET_MODULE_MEMORY_TYPE
TEST_F(DMTestModule, test_GET_MODULE_MEMORY_TYPE) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "ETLPDDR4", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

/* Note : This test should be run in full boot mode only.
// Test GET_FIRMWARE_BOOT_STATUS
TEST_F(DMTestModule, test_GET_FIRMWARE_BOOT_STATUS) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_FIRMWARE_BOOT_STATUS, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "0", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/

// Test unsupported service
TEST_F(DMTestModule, test_unsupported_service) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  //Invalid device node
  ASSERT_EQ(dm.serviceRequest("et_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), -EINVAL);
  ASSERT_EQ(dm.serviceRequest("et0_mgmts", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), -EINVAL);

  //Invalid command code
  ASSERT_EQ(dm.serviceRequest("et0_mgmt", 99999, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), -EINVAL);

  //Invalid input_buffer (not supported yet as WP1 only supports GET codes) 
  //ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), -EINVAL);

  // Invalid output_buffer
  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, nullptr, output_size, hst_latency.get(), dev_latency.get(), 0), -EINVAL);

  //Invalid host latency
  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, nullptr, dev_latency.get(), 0), -EINVAL);

  //Invalid device latency
  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MEMORY_TYPE, nullptr, 0, output_buff, output_size, hst_latency.get(), nullptr, 0), -EINVAL);
}

void testSerial(DeviceManagement &dm, uint32_t timeout, int *result) {
  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  *result = dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_MANUFACTURE_NAME, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), timeout);
}

// Test serial access
TEST_F(DMTestModule, test_serial_access) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  auto res1 = std::make_unique<int>();
  auto res2 = std::make_unique<int>();
  auto res3 = std::make_unique<int>();

  std::thread first(testSerial, std::ref(dm), (uint32_t)1000, res1.get());
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  std::thread second(testSerial, std::ref(dm), (uint32_t)0, res2.get());
  std::thread third(testSerial, std::ref(dm), (uint32_t)70000, res3.get());

  first.join();
  second.join();
  third.join();

  ASSERT_EQ(*res1, 0);
  ASSERT_EQ(*res2, -EAGAIN);
  ASSERT_EQ(*res3, 0);
}

/* TODO : Enable Firmware update test case 
// copy /projects/esperanto/img_v002/ to your sw-platform directory and change IMG_USER below
#define IMG_PRE "/projects/esperanto/"
#define IMG_USER "mpowell"
#define IMG_POST "/sw-platform/img_v002"
#define SP_CRT_512_V002 IMG_PRE IMG_USER IMG_POST "/hash.txt"
#define IMG_V002 IMG_PRE IMG_USER IMG_POST "/flash_16Mbit.img"

// Test SET_FIRMWARE_UPDATE
TEST_F(DMTestModule, test_SET_FIRMWARE_UPDATE) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::SET_FIRMWARE_UPDATE, IMG_V002, 1, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "0", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/

/* TODO : Enable SP ROO CERT Hash provision test
// Test SET_SP_BOOT_ROOT_CERT
TEST_F(DMTestModule, test_SET_SP_BOOT_ROOT_CERT) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1 , output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "0", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}
*/

/* TODO : Enable Firmware update test case 
// Test firmware_update
TEST_F(DMTestModule, test_firmware_update) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  // Step 1: GET_MODULE_FIRMWARE_REVISIONS
  const uint32_t rev1_output_size = 20;
  char rev1_output_buff[rev1_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0, rev1_output_buff, rev1_output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("rev1_output_buff: %.*s\n", rev1_output_size, rev1_output_buff);

  char rev1_expected[rev1_output_size] = {0};
  strncpy(rev1_expected, "00100010001000100010", rev1_output_size);
  printf("rev1_expected: %.*s\n", rev1_output_size, rev1_expected);
  ASSERT_EQ(strncmp(rev1_output_buff, rev1_expected, rev1_output_size), 0);

  // Step 2: SET_SP_BOOT_ROOT_CERT
  const uint32_t crt_output_size = 8;
  char crt_output_buff[crt_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1 , crt_output_buff, crt_output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("crt_output_buff: %.*s\n", crt_output_size, crt_output_buff);

  char crt_expected[crt_output_size] = {0};
  strncpy(crt_expected, "0", crt_output_size);
  printf("crt_expected: %.*s\n", crt_output_size, crt_expected);
  ASSERT_EQ(strncmp(crt_output_buff, crt_expected, crt_output_size), 0);

  // Step 3: SET_FIRMWARE_UPDATE
  const uint32_t fw_output_size = 8;
  char fw_output_buff[fw_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::SET_FIRMWARE_UPDATE, IMG_V002, 1, fw_output_buff, fw_output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("fw_output_buff: %.*s\n", fw_output_size, fw_output_buff);

  char fw_expected[fw_output_size] = {0};
  strncpy(fw_expected, "0", fw_output_size);
  printf("fw_expected: %.*s\n", fw_output_size, fw_expected);
  ASSERT_EQ(strncmp(fw_output_buff, fw_expected, fw_output_size), 0);
}

// Test verify_firmware_update (Manual continutation after SP resets)
TEST_F(DMTestModule, test_verify_firmware_update) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  // Step 4: GET_FIRMWARE_BOOT_STATUS
  const uint32_t bt_output_size = 8;
  char bt_output_buff[bt_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_FIRMWARE_BOOT_STATUS, nullptr, 0, bt_output_buff, bt_output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("bt_output_buff: %.*s\n", bt_output_size, bt_output_buff);

  char bt_expected[bt_output_size] = {0};
  strncpy(bt_expected, "0", bt_output_size);
  printf("bt_expected: %.*s\n", bt_output_size, bt_expected);
  ASSERT_EQ(strncmp(bt_output_buff, bt_expected, bt_output_size), 0);

  // Step 5: GET_MODULE_FIRMWARE_REVISIONS
  const uint32_t rev2_output_size = 20;
  char rev2_output_buff[rev2_output_size] = {0};

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0, rev2_output_buff, rev2_output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("rev2_output_buff: %.*s\n", rev2_output_size, rev2_output_buff);

  char rev2_expected[rev2_output_size] = {0};
  strncpy(rev2_expected, "00200020002000200020", rev2_output_size);
  printf("rev2_expected: %.*s\n", rev2_output_size, rev2_expected);
  ASSERT_EQ(strncmp(rev2_output_buff, rev2_expected, rev2_output_size), 0);
}
*/

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
