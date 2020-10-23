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
  void TearDown() override { dlclose(&handle_); }

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

// Test GET_MODULE_FIRMWARE_REVISIONS
TEST_F(DMTestModule, test_GET_MODULE_FIRMWARE_REVISIONS) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement &dm = (*dmi)();

  const uint32_t output_size = 8;
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ASSERT_EQ(dm.serviceRequest("et0_mgmt", CommandCode::GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(), 0), 0);
  printf("output_buff: %.*s\n", output_size, output_buff);

  char expected[output_size] = {0};
  strncpy(expected, "v-0.0.1", output_size);
  printf("expected: %.*s\n", output_size, expected);
  ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
}

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
  std::thread third(testSerial, std::ref(dm), (uint32_t)30000, res3.get());

  first.join();
  second.join();
  third.join();

  ASSERT_EQ(*res1, 0);
  ASSERT_EQ(*res2, -EAGAIN);
  ASSERT_EQ(*res3, 0);

}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
