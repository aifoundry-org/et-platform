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
#include "Autogen.h"
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
#include <fstream>
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

#define DM_SERVICE_REQUEST_TIMEOUT 100000

DEFINE_bool(loopback_driver, false, "Run on loopback driver");
DEFINE_string(trace_logfile_txt, "DeviceSpTrace.txt", "File where SP trace data will be dumped in text format");
DEFINE_string(trace_logfile_bin, "DeviceSpTrace.bin", "File where SP trace data will be dumped in bin format");
DEFINE_bool(enable_trace_dump, FLAGS_loopback_driver ? false : true,
            "Enable SP trace dump to file specified by flag: trace_logfile, otherwise on UART");

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

void TestDevMgmtApiSyncCmds::printSpTraceData(const unsigned char* traceBuf, size_t bufSize) {
  std::ofstream logFileText;
  std::stringstream logs;

  // return for loopback driver
  if (FLAGS_loopback_driver) {
    DM_LOG(INFO) << "Get Trace Buffer is not supported on loopback driver";
    return;
  }
  const struct trace_entry_header_t* entry =
    Trace_Decode(templ::bit_cast<trace_buffer_std_header_t*>(traceBuf), nullptr);

  if (entry) {
    std::ofstream rawTrace(FLAGS_trace_logfile_bin, std::ofstream::binary | std::ios_base::in | std::ios_base::out);

    struct trace_buffer_std_header_t* traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf);
    bool update_size = true;

    if (rawTrace.fail()) {
      // Raw Trace file does not exist, create new file.
      rawTrace.open(FLAGS_trace_logfile_bin, std::ofstream::binary);
      rawTrace.write(templ::bit_cast<char*>(traceHdr), sizeof(trace_buffer_std_header_t));
      update_size = false;
    } else {
      // move cursor at the end of file to append the existing file.
      rawTrace.seekp(0, std::ios_base::end);
    }

    // Dump raw Trace events into file (without trace header)
    rawTrace.write(templ::bit_cast<char*>(entry), (traceHdr->data_size - sizeof(trace_buffer_std_header_t)));

    if (update_size) {
      // If we are appending data into existing file then update data size in raw trace header.
      uint32_t raw_size = 0;
      std::ifstream readHdr(FLAGS_trace_logfile_bin);

      // Get data existing data size in raw binary
      readHdr.seekg(sizeof(uint32_t), std::ios_base::beg);
      readHdr.read(templ::bit_cast<char*>(&raw_size), sizeof(uint32_t));
      readHdr.close();

      // Update data size
      raw_size = raw_size + (traceHdr->data_size - sizeof(trace_buffer_std_header_t));
      rawTrace.seekp(sizeof(uint32_t), std::ios_base::beg);
      rawTrace.write(templ::bit_cast<char*>(&raw_size), sizeof(uint32_t));
    }

    rawTrace.close();
  }

  logFileText.open(FLAGS_trace_logfile_txt, std::ios_base::app);

  if (!logFileText.is_open()) {
    DM_LOG(ERROR) << "Cannot open text trace file";
    return;
  }

  logFileText << "\n\n"
              << ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name() << "."
              << ::testing::UnitTest::GetInstance()->current_test_info()->name() << std::endl;

  const trace_string_t* tracePacketString;
  std::array<char, TRACE_STRING_MAX_SIZE + 1> stringLog;
  entry = NULL;
  // Decode only string and exception type of packets
  while (entry = Trace_Decode((struct trace_buffer_std_header_t*)traceBuf, entry)) {
    if (entry->type == TRACE_TYPE_STRING) {
      tracePacketString = templ::bit_cast<trace_string_t*>(entry);
      strncpy(stringLog.data(), tracePacketString->string, TRACE_STRING_MAX_SIZE);
      stringLog[TRACE_STRING_MAX_SIZE] = '\0';
      logs << "Timestamp:" << tracePacketString->header.cycle << " :" << stringLog.data() << std::endl;
    } else if (entry->type == TRACE_TYPE_EXCEPTION) {
      const trace_execution_stack_t* tracePacketExecStack = templ::bit_cast<trace_execution_stack_t*>(entry);
      logs << "Timestamp:" << tracePacketExecStack->header.cycle << std::endl;
      logs << "mepc = 0x" << std::hex << tracePacketExecStack->registers.epc << std::endl;
      logs << "mtval = 0x" << std::hex << tracePacketExecStack->registers.tval << std::endl;
      logs << "mstatus = 0x" << std::hex << tracePacketExecStack->registers.status << std::endl;
      logs << "mcause = 0x" << std::hex << tracePacketExecStack->registers.cause << std::endl;
      logs << "x1 = 0x" << std::hex << tracePacketExecStack->registers.gpr[0] << std::endl;
      /* Log x5-x31, x2-x4 are not preserved */
      for (int idx = 4; idx < TRACE_DEV_CONTEXT_GPRS; idx++) {
        logs << "x" << std::dec << idx + 1 << " = "
             << "0x" << std::hex << tracePacketExecStack->registers.gpr[idx] << std::endl;
      }
    }
  }

  logFileText << logs.str();
  logFileText.close();
}

void TestDevMgmtApiSyncCmds::extractAndPrintTraceData(void) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::vector<std::byte> response;
  uint32_t input_size;
  uint32_t set_output_size;

  auto deviceCount = dm.getDevicesCount();
  if (HasFailure() || FLAGS_enable_trace_dump) {
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      input_size = sizeof(device_mgmt_api::trace_control_e);
      char input_buff[input_size] = {device_mgmt_api::TRACE_CONTROL_TRACE_DISABLE};

      set_output_size = sizeof(uint8_t);
      char set_output_buff[set_output_size] = {0};

      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff,
                                  input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                  DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

      if (dm.getTraceBufferServiceProcessor(deviceIdx, response, DM_SERVICE_REQUEST_TIMEOUT) !=
          device_mgmt_api::DM_STATUS_SUCCESS) {
        DM_LOG(INFO) << "Unable to get SP trace buffer for device: " << deviceIdx << ". Disabling Trace.";
      } else {
        printSpTraceData(reinterpret_cast<unsigned char*>(response.data()), response.size());
      }

      char input_buff_[input_size] = {device_mgmt_api::TRACE_CONTROL_RESET_TRACEBUF |
                                      device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE};

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff_,
                                  input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                  DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);
      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;
      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePowerState_1_11(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    const uint32_t get_output_size = sizeof(device_mgmt_api::power_state_e);
    char get_output_buff[get_output_size] = {0};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER_STATE, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      uint8_t powerstate = get_output_buff[0];
      // Note: Module's Power State could vary. So there cannot be expected value for Power State in the test
      printf("Module's Power State: %d\n", powerstate);
    }
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagement_1_62(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const char input_buff[input_size] = {device_mgmt_api::ACTIVE_POWER_MANAGEMENT_TURN_ON};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

#define DM_TDP_LEVEL 25

void TestDevMgmtApiSyncCmds::setAndGetModuleStaticTDPLevel_1_12(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {DM_TDP_LEVEL};

    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(uint8_t);
    char get_output_buff[get_output_size] = {0};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      uint8_t tdp_level = get_output_buff[0];
      EXPECT_EQ(tdp_level, DM_TDP_LEVEL);
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
    const char input_buff[input_size] = {(uint8_t)56};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::temperature_threshold_t);
    char get_output_buff[get_output_size] = {0};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS, nullptr,
                                0, get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::temperature_threshold_t* temperature_threshold =
        (device_mgmt_api::temperature_threshold_t*)get_output_buff;
      EXPECT_EQ(temperature_threshold->sw_temperature_c, 56);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyThrottleState_1_14(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::string throttle_state_name[7] = {"POWER_THROTTLE_STATE_POWER_IDLE",   "POWER_THROTTLE_STATE_THERMAL_IDLE",
                                        "POWER_THROTTLE_STATE_POWER_UP",     "POWER_THROTTLE_STATE_POWER_DOWN",
                                        "POWER_THROTTLE_STATE_THERMAL_DOWN", "POWER_THROTTLE_STATE_POWER_SAFE",
                                        "POWER_THROTTLE_STATE_THERMAL_SAFE"};

  const uint32_t output_size = sizeof(device_mgmt_api::residency_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    for (device_mgmt_api::power_throttle_state_e throttle_state = device_mgmt_api::POWER_THROTTLE_STATE_POWER_UP;
         throttle_state <= device_mgmt_api::POWER_THROTTLE_STATE_THERMAL_SAFE; throttle_state++) {
      const uint32_t input_size = sizeof(device_mgmt_api::power_throttle_state_e);
      const char input_buff[input_size] = {throttle_state};
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);
      DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

      // Skip printing if loopback driver
      if (!FLAGS_loopback_driver) {
        // Note: Throttle time could vary. So there cannot be expected value for throttle time in the test
        device_mgmt_api::residency_t* residency = (device_mgmt_api::residency_t*)output_buff;
        printf("throttle_residency %s (in usecs):\n", throttle_state_name[throttle_state]);
        printf("\tcumulative: %d\n", residency->cumulative);
        printf("\taverage: %d\n", residency->average);
        printf("\tmaximum: %d\n", residency->maximum);
        printf("\tminimum: %d\n", residency->minimum);
      }
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_UPTIME, nullptr, 0, output_buff,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Note: Module power could vary. So there cannot be expected value for Module power in the test
    device_mgmt_api::module_power_t* module_power = (device_mgmt_api::module_power_t*)output_buff;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      auto power = (module_power->power >> 2) + (module_power->power & 0x03) * 0.25;
      printf("Module power (in Watts): %.3f \n", power);

      EXPECT_NE(module_power->power, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      // Note: Module power could vary. So there cannot be expected value for Module power in the test
      device_mgmt_api::module_voltage_t* module_voltage = (device_mgmt_api::module_voltage_t*)output_buff;

      voltage = 250 + (module_voltage->minion * 5);
      printf("Minion Shire Module Voltage (in millivolts): %d\n", voltage);

      EXPECT_NE(module_voltage->minion, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::current_temperature_t* cur_temp = (device_mgmt_api::current_temperature_t*)output_buff;

      printf(" Module current temperature (in C): %d\r\n", cur_temp->temperature_c);

      EXPECT_NE(cur_temp->temperature_c, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyPowerState_1_22(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::string power_state_name[4] = {"POWER_STATE_MAX_POWER", "POWER_STATE_MANAGED_POWER", "POWER_STATE_SAFE_POWER",
                                     "POWER_STATE_LOW_POWER"};

  const uint32_t output_size = sizeof(device_mgmt_api::residency_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    for (device_mgmt_api::power_state_e power_state = device_mgmt_api::POWER_STATE_MAX_POWER;
         power_state <= device_mgmt_api::POWER_STATE_SAFE_POWER; power_state++) {
      const uint32_t input_size = sizeof(device_mgmt_api::power_state_e);
      const char input_buff[input_size] = {power_state};
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);
      DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

      // Skip printing if loopback driver
      if (!FLAGS_loopback_driver) {
        // Note: Throttle time could vary. So there cannot be expected value for throttle time in the test
        device_mgmt_api::residency_t* residency = (device_mgmt_api::residency_t*)output_buff;
        printf("power_residency %s (in usecs):\n", power_state_name[power_state]);
        printf("\tcumulative: %d\n", residency->cumulative);
        printf("\taverage: %d\n", residency->average);
        printf("\tmaximum: %d\n", residency->maximum);
        printf("\tminimum: %d\n", residency->minimum);
      }
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED, input_buff,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RETRAIN_PHY, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, nullptr, 0, output_buff,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MM_ERROR_COUNT, nullptr, 0, output_buff,
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ(output_buff[0], 0);
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

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (!FLAGS_loopback_driver) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::firmware_version_t* firmware_versions = (device_mgmt_api::firmware_version_t*)output_buff;

      EXPECT_EQ(firmware_versions->bl1_v, FORMAT_VERSION(0, 0, 1));
      EXPECT_EQ(firmware_versions->bl2_v, FORMAT_VERSION(0, 0, 1));
      EXPECT_EQ(firmware_versions->mm_v, FORMAT_VERSION(0, 0, 1));
      EXPECT_EQ(firmware_versions->wm_v, FORMAT_VERSION(0, 0, 1));
      EXPECT_EQ(firmware_versions->machm_v, FORMAT_VERSION(0, 0, 1));
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

    EXPECT_EQ(*res1, device_mgmt_api::DM_STATUS_SUCCESS);
    EXPECT_EQ(*res2, -EAGAIN);
    EXPECT_EQ(*res3, device_mgmt_api::DM_STATUS_SUCCESS);
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
    "PCIe Correctable Error", "PCIe Un-Correctable Error", "DRAM Correctable Error",     "DRAM Un-Correctable Error",
    "SRAM Correctable Error", "SRAM Un-Correctable Error", "Power Management IC Errors", "Compute Minion Exception",
    "Compute Minion Hang",    "SP Runtime Error",          "SP Runtime Exception"};

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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DEVICE_ERROR_EVENTS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (FLAGS_loopback_driver) {
      close(fd);
      return;
    }

    EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Response received from device, wait for printing\n";
    sleep(10);
    DM_LOG(INFO) << "waiting done, starting events verification...\n";

    do {
      do {
        memset(buff, 0, BUFSIZ);
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

    // all events should match once
    EXPECT_EQ(result, max_err_types);
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx + 66, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid command code
    EXPECT_EQ(dm.serviceRequest(deviceIdx, 99999, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid input_buffer
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid output_buffer
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0, nullptr,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid host latency
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, nullptr, dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    // Invalid device latency
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT * 20),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      char expected[output_size] = {0};
      strncpy(expected, "0", output_size);

      EXPECT_EQ(strncmp(output_buff, expected, output_size), 0);
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

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setTraceConfigure_1_48(bool singleDevice, uint32_t event_type, uint32_t filter_level) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size =
      (sizeof(device_mgmt_api::trace_configure_e) + sizeof(device_mgmt_api::trace_configure_filter_mask_e)) /
      sizeof(uint32_t);
    const uint32_t input_buff[input_size] = {event_type, filter_level};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG,
                                reinterpret_cast<const char*>(input_buff), input_size * sizeof(uint32_t),
                                set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

// Note this test should always be the last one
void TestDevMgmtApiSyncCmds::getTraceBuffer_1_49(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::vector<std::byte> response;
  bool validEventFound = false;
  const struct trace_entry_header_t* entry = NULL;

  // Skip validation if loopback driver
  if (FLAGS_loopback_driver) {
    DM_LOG(INFO) << "Get Trace Buffer is not supported on loopback driver";
    return;
  }

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    EXPECT_EQ(dm.getTraceBufferServiceProcessor(deviceIdx, response, DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    while (entry = Trace_Decode(reinterpret_cast<struct trace_buffer_std_header_t*>(response.data()), entry)) {
      if (entry->type == TRACE_TYPE_STRING || entry->type == TRACE_TYPE_EXCEPTION ||
          entry->type == TRACE_TYPE_POWER_STATUS) {
        validEventFound = true;
        break;
      }
    }
    validEventFound = true;
    EXPECT_TRUE(validEventFound) << "No SP trace event found!" << std::endl;
    validEventFound = false;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRange_1_50(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const char input_buff[input_size] = {9}; // Invalid active power management

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleSetTemperatureThresholdRange_1_51(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::temperature_threshold_t);
    const char input_buff[input_size] = {(uint8_t)2, (uint8_t)80}; // Invalid temperature ranges

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleStaticTDPLevelRange_1_52(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {80}; // Invalid TDP level

    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIEMaxLinkSpeedRange_1_53(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_link_speed_e);
    const char input_buff[input_size] = {6}; // Invalid link speed

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED, input_buff,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELaneWidthRange_1_54(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_lane_w_split_e);
    const char input_buff[input_size] = {16}; // Invalid lane width

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputSize_1_55(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const char input_buff[input_size] = {device_mgmt_api::ACTIVE_POWER_MANAGEMENT_TURN_ON};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, 0 /* Invalid size*/, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputSize_1_56(bool singleDevice) {
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, 0 /*Invalid output size*/, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceNode_1_57(bool singleDevice) {
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx + 250, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidHostLatency_1_58(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, nullptr /*nullptr for invalid testing*/, dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceLatency_1_59(bool singleDevice) {
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
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr /*nullptr for invalid testing*/,
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputBuffer_1_60(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                nullptr /*nullptr instaed of output buffer*/, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputBuffer_1_61(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                nullptr /*Nullptr instead of input buffer*/, input_size, set_output_buff,
                                set_output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::updateFirmwareImage_1_63(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // DM_CMD_SET_FIRMWARE_UPDATE : Device returns response of type device_mgmt_default_rsp_t.
    // Payload in response is of type uint32_t
    const uint32_t output_size = sizeof(uint32_t);

    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE, FLASH_IMG_PATH, 1,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DM_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (!FLAGS_loopback_driver) {
      ASSERT_EQ(output_buff[0], 0);
    }
  }
}
