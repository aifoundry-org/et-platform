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
#include "TestDevMgmtApi.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <dlfcn.h>
#include <errno.h>
#include <experimental/filesystem>
#include <fcntl.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::experimental::filesystem;

using namespace dev;
using namespace device_management;
using namespace std::chrono_literals;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

#define DM_SERVICE_REQUEST_TIMEOUT 70000

DEFINE_bool(loopback_driver, false, "Run on loopback driver");

#define FORMAT_VERSION(major, minor, revision) ((major << 24) | (minor << 16) | (revision << 8))

void testSerial(DeviceManagement& dm, uint32_t deviceIdx, uint32_t index, uint32_t timeout, int* result) {
  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char output_buff[output_size] = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  if (index == 2 || index == 3) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  *result = dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                              output_buff, output_size, hst_latency.get(), dev_latency.get(), timeout);
}

getDM_t TestDevMgmtApiSyncCmds::getInstance() {
  const char* error;

  if (handle_) {
    getDM_t getDM = reinterpret_cast<getDM_t>(dlsym(handle_, "getInstance"));
    if (!(error = dlerror())) {
      return getDM;
    }
    std::cout << "Error: " << error << std::endl;
  }
  return (getDM_t)0;
}

void TestDevMgmtApiSyncCmds::getModuleManufactureName_1_1(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);
      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;
      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePartNumber_1_2(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "ETPART1", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleSerialNumber_1_3(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "ETSER_1", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICChipRevision_1_4(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "160", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePCIENumPortsMaxSpeed_1_5(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "8", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemorySizeMB_1_6(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "16384", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleRevision_1_7(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "1", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleFormFactor_1_8(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Dual_M2", output_size);
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemoryVendorPartNumber_1_9(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Unknown", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemoryType_1_10(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "LPDDR4X", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      ASSERT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModulePowerState_1_11(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::power_state_e);
    const char input_buff[input_size] = {device_mgmt_api::POWER_STATE_REDUCED};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_POWER_STATE, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::power_state_e);
    char get_output_buff[get_output_size] = {0};

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER_STATE, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      uint8_t powerstate = get_output_buff[0];
      ASSERT_EQ(powerstate, device_mgmt_api::POWER_STATE_REDUCED);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModuleStaticTDPLevel_1_12(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::tdp_level_e);
    const char input_buff[input_size] = {device_mgmt_api::TDP_LEVEL_TWO};

    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::tdp_level_e);
    char get_output_buff[get_output_size] = {0};

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      uint8_t tdp_level = get_output_buff[0];
      ASSERT_EQ(tdp_level, device_mgmt_api::TDP_LEVEL_TWO);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModuleTemperatureThreshold_1_13(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::temperature_threshold_t);
    const char input_buff[input_size] = {(uint8_t)56, (uint8_t)84};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::temperature_threshold_t);
    char get_output_buff[get_output_size] = {0};

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS, nullptr,
                                0, get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::temperature_threshold_t* temperature_threshold =
        (device_mgmt_api::temperature_threshold_t*)get_output_buff;
      ASSERT_EQ(temperature_threshold->lo_temperature_c, 56);
      ASSERT_EQ(temperature_threshold->hi_temperature_c, 84);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyThrottleState_1_14(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::throttle_time_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (!FLAGS_loopback_driver) {
      // Note: Throttle time could vary. So there cannot be expected value for throttle time in the test
      device_mgmt_api::throttle_time_t* throttle_time = (device_mgmt_api::throttle_time_t*)output_buff;
      printf("throttle_time (in usecs): %d\n", throttle_time->time_usec);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleUptime_1_15(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::module_uptime_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_UPTIME, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::module_uptime_t* module_uptime = (device_mgmt_api::module_uptime_t*)output_buff;
      printf("Module uptime (day:hours:mins): %d:%d:%d\r\n", module_uptime->day, module_uptime->hours,
             module_uptime->mins);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePower_1_16(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::module_power_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Note: Module power could vary. So there cannot be expected value for Module power in the test
    device_mgmt_api::module_power_t* module_power = (device_mgmt_api::module_power_t*)output_buff;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      auto power = (module_power->power >> 2) + (module_power->power & 0x03) * 0.25;
      printf("Module power (in Watts): %.3f \n", power);

      ASSERT_NE(module_power->power, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleVoltage_1_17(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::module_voltage_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    uint32_t voltage;

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      // Note: Module power could vary. So there cannot be expected value for Module power in the test
      device_mgmt_api::module_voltage_t* module_voltage = (device_mgmt_api::module_voltage_t*)output_buff;

      voltage = 250 + (module_voltage->minion * 5);
      printf("Minion Shire Module Voltage (in millivolts): %d\n", voltage);

      ASSERT_NE(module_voltage->minion, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleCurrentTemperature_1_18(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::current_temperature_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::current_temperature_t* cur_temp = (device_mgmt_api::current_temperature_t*)output_buff;

      printf(" Module current temperature (in C): %d\r\n", cur_temp->temperature_c);

      ASSERT_NE(cur_temp->temperature_c, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxTemperature_1_19(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::max_temperature_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::max_temperature_t* max_temperature = (device_mgmt_api::max_temperature_t*)output_buff;

      // Note: Module's Max Temperature could vary. So there cannot be expected value for max temperature in the test
      printf("Module's Max Temperature: %d\n", max_temperature->max_temperature_c);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxMemoryErrors_1_20(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::max_ecc_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::max_ecc_count_t* max_ecc_count = (device_mgmt_api::max_ecc_count_t*)output_buff;

      // Note: ECC count could vary. So there cannot be expected value for max_ecc_count in the test
      printf("Max ECC Count: %d\n", max_ecc_count->count);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxDDRBW_1_21(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::max_dram_bw_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxThrottleTime_1_22(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::max_throttle_time_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_THROTTLE_TIME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::max_throttle_time_t* max_throttle_time = (device_mgmt_api::max_throttle_time_t*)output_buff;

      // Note: Throttle time could vary so there cannot be an expected value.
      // ASSERT_EQ(max_throttle_time->time_usec, 1000);
      printf("Max Throttle Time: %llu\n", max_throttle_time->time_usec);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetDDRECCThresholdCount_1_23(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {10};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetSRAMECCThresholdCount_1_24(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {20};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetPCIEECCThresholdCount_1_25(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {30};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getPCIEECCUECCCount_1_26(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      ASSERT_EQ(errors_count->ecc, 0);
      ASSERT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getDDRECCUECCCount_1_27(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      ASSERT_EQ(errors_count->ecc, 0);
      ASSERT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getSRAMECCUECCCount_1_28(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      ASSERT_EQ(errors_count->ecc, 0);
      ASSERT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getDDRBWCounter_1_29(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dram_bw_counter_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELinkSpeed_1_30(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_link_speed_e);
    const char input_buff[input_size] = {device_mgmt_api::PCIE_LINK_SPEED_GEN3};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED, input_buff,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIELaneWidth_1_31(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_lane_w_split_e);
    const char input_buff[input_size] = {device_mgmt_api::PCIE_LANE_W_SPLIT_x4};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIERetrainPhy_1_32(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {0};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RETRAIN_PHY, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICFrequencies_1_33(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asic_frequencies_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMBW_1_34(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::dram_bw_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilization_1_35(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::percentage_cap_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilization_1_36(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICUtilization_1_37(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICStalls_1_38(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICLatency_1_39(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getMMErrorCount_1_40(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::mm_error_count_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MM_ERROR_COUNT, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

  }
}

void TestDevMgmtApiSyncCmds::getFWBootstatus_1_41(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // DM_CMD_GET_FIRMWARE_BOOT_STATUS : Device returns response of type device_mgmt_default_rsp_t.
    // Payload in response is of type uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleFWRevision_1_42(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::firmware_version_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::firmware_version_t* firmware_versions = (device_mgmt_api::firmware_version_t*)output_buff;

      ASSERT_EQ(firmware_versions->bl1_v, FORMAT_VERSION(0, 0, 1));
      ASSERT_EQ(firmware_versions->bl2_v, FORMAT_VERSION(0, 0, 1));
      ASSERT_EQ(firmware_versions->mm_v, FORMAT_VERSION(3, 17, 3));
      ASSERT_EQ(firmware_versions->wm_v, FORMAT_VERSION(3, 17, 3));
      ASSERT_EQ(firmware_versions->machm_v, FORMAT_VERSION(3, 17, 3));
    }
  }
}

void TestDevMgmtApiSyncCmds::serializeAccessMgmtNode_1_43(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto res1 = std::make_unique<int>();
    auto res2 = std::make_unique<int>();
    auto res3 = std::make_unique<int>();

    std::thread first(testSerial, std::ref(dm), (uint32_t)deviceIdx, (uint32_t)1, (uint32_t)DM_SERVICE_REQUEST_TIMEOUT,
                      res1.get());
    std::thread second(testSerial, std::ref(dm), (uint32_t)deviceIdx, (uint32_t)2, (uint32_t)0, res2.get());
    std::thread third(testSerial, std::ref(dm), (uint32_t)deviceIdx, (uint32_t)3,
                      (uint32_t)DM_SERVICE_REQUEST_TIMEOUT << 1, res3.get());

    first.join();
    second.join();
    third.join();

    ASSERT_EQ(*res1, device_mgmt_api::DM_STATUS_SUCCESS);
    ASSERT_EQ(*res2, -EAGAIN);
    ASSERT_EQ(*res3, device_mgmt_api::DM_STATUS_SUCCESS);
  }
}

void TestDevMgmtApiSyncCmds::getDeviceErrorEvents_1_44(bool singleDevice) {
  int fd, i;
  char buff[BUFSIZ];
  ssize_t size = 0;
  int result = 0;
  int mode = O_RDONLY | O_NONBLOCK;
  const int max_err_types = 11;
  std::string line;
  std::string err_types[max_err_types] = {
    "PCIe Correctable Error",        "PCIe Un-Correctable Error",  "DRAM Correctable Error",
    "DRAM Un-Correctable Error",     "SRAM Correctable Error",     "SRAM Un-Correctable Error",
    "Temperature Overshoot Warning", "Power Management IC Errors", "Thermal Throttling Error",
    "Compute Minion Exception",      "Compute Minion Hang"};

  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    int err_count[max_err_types] = {0};
    result = 0;
    size = 0;
    fd = open("/dev/kmsg", mode);
    lseek(fd, 0, SEEK_END);
    DM_LOG(INFO) << "waiting for error events...\n";
    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DEVICE_ERROR_EVENTS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (FLAGS_loopback_driver) {
      close(fd);
      return;
    }

    ASSERT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Response received from device, wait for printing\n";
    sleep(10);
    DM_LOG(INFO) << "waiting done, starting events verification...\n";

    do {
      do {
        buff[0] = '\0';
        size = read(fd, buff, BUFSIZ - 1);
      } while (size < 0 && errno == EPIPE);

      line.assign(buff);
      for (i = 0; i < max_err_types; i++) {
        if (std::string::npos != line.find(err_types[i])) {
          err_count[i] += 1;
          break;
        }
      }
    } while (size > 0);

    for (i = 0; i < max_err_types; i++) {
      DM_LOG(INFO) << "matched '" << err_types[i] << "' " << err_count[i] << " time(s)\n";
      if (err_count[i] > 0) {
        result += err_count[i];
      }
    }

    close(fd);

    // Eleven events, and each should match once.
    ASSERT_EQ(result, 9);
  }
}

void TestDevMgmtApiSyncCmds::isUnsupportedService_1_45(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    // Invalid device node
    ASSERT_EQ(dm.serviceRequest(deviceIdx + 66, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid command code
    ASSERT_EQ(dm.serviceRequest(deviceIdx, 99999, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid input_buffer
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_POWER_STATE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid output_buffer
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0, nullptr,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid host latency
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, nullptr, dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid device latency
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr, DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Requests Completed for Device: " << deviceIdx;
  }
}

#define SP_CRT_512_V002 "../include/hash.txt"

void TestDevMgmtApiSyncCmds::setSpRootCertificate_1_46(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Payload in response is of type uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT * 20),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      char expected[output_size] = {0};
      strncpy(expected, "0", output_size);

      ASSERT_EQ(strncmp(output_buff, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::setTraceControl_1_47(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::trace_control_e);
    const char input_buff[input_size] = {device_mgmt_api::TRACE_CONTROL_TRACE_UART_DISABLE};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setTraceConfigure_1_48(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size =
      (sizeof(device_mgmt_api::trace_configure_e) + sizeof(device_mgmt_api::trace_configure_filter_mask_e)) /
      sizeof(uint32_t);
    const uint32_t input_buff[input_size] = {device_mgmt_api::TRACE_CONFIGURE_EVENT_STRING,
                                             device_mgmt_api::TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG,
                                reinterpret_cast<const char*>(input_buff), input_size * sizeof(uint32_t),
                                set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getTraceBuffer_1_49(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::vector<std::byte> response;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    ASSERT_EQ(dm.getTraceBufferServiceProcessor(deviceIdx, response, DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      bool validStringEventFound = false;
#if SW_8496
      // Get Trace buffer header
      auto traceHeader = templ::bit_cast<trace_buffer_std_header_t*>(response.data());
      // TODO: enable after SW-8496
      // Check if it is valid SP Trace buffer.
      if ((traceHeader->magic_header != TRACE_MAGIC_HEADER) || (traceHeader->type != TRACE_SP_BUFFER) ||
          ((traceHeader->data_size) > response.size())) {
        DM_LOG(ERROR) << "Invalid SP Trace Buffer!";
        break;
      }
      // Get size from Trace buffer header
      size_t dataSize = traceHeader->data_size - sizeof(struct trace_buffer_std_header_t);
#endif
      size_t dataSize = response.size() - sizeof(struct trace_buffer_std_header_t);
      auto traceBuf = response.data() + sizeof(struct trace_buffer_std_header_t);
      auto dataPtr = templ::bit_cast<trace_string_t*>(traceBuf);
      char stringLog[TRACE_STRING_MAX_SIZE + 1]; // NOSONAR this is required for SP trace buffer parsing.
      size_t dataPopped = 0;
      DM_LOG(INFO) << "Printing trace buffer:";
      while (dataPopped < dataSize) {
        if ((dataPtr->header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')) {
          strncpy(&stringLog[0], dataPtr->dataString, TRACE_STRING_MAX_SIZE);
          stringLog[TRACE_STRING_MAX_SIZE] = '\0';
          std::cout << stringLog << std::endl;
          dataPtr++;
          dataPopped += sizeof(struct trace_string_t);
          validStringEventFound = true;
        } else {
          break;
        }
      }
      ASSERT_EQ(validStringEventFound, true);
    }
  }
}
