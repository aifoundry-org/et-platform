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

DEFINE_bool(enable_trace_dump, true,
            "Enable SP trace dump to file specified by flag: trace_logfile, otherwise on UART");
DEFINE_bool(reset_trace_buffer, true,
            "Reset the SP trace buffer on the start of the test run if trace logging is enabled");
DEFINE_string(trace_base_dir, "devtrace", "Base directory which will contain all traces");
DEFINE_string(trace_txt_dir, FLAGS_trace_base_dir + "/txt_files",
              "A directory in the current path where the decoded device traces will be printed");
DEFINE_string(trace_bin_dir, FLAGS_trace_base_dir + "/bin_files",
              "A directory in the current path where the raw device traces will be dumped");

#define FORMAT_VERSION(major, minor, revision) ((major << 24) | (minor << 16) | (revision << 8))

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

std::string inline getTraceTxtName(int deviceIdx) {
  return (fs::path(FLAGS_trace_txt_dir) / fs::path("dev" + std::to_string(deviceIdx) + "_traces.txt")).string();
}

std::string inline getFullTestName() {
  return ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name() + std::string(".") +
         ::testing::UnitTest::GetInstance()->current_test_info()->name();
}

void TestDevMgmtApiSyncCmds::initTestTrace() {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();

  if (getTestTarget() == Target::Loopback) {
    FLAGS_enable_trace_dump = false;
  }
  if (FLAGS_enable_trace_dump) {
    fs::create_directory(FLAGS_trace_base_dir);
    fs::create_directory(FLAGS_trace_txt_dir);
    fs::create_directory(FLAGS_trace_bin_dir);
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      std::ofstream logfile(getTraceTxtName(deviceIdx), std::ios_base::app);
      logfile << "\n" << getFullTestName() << std::endl;
    }
  }
}

void TestDevMgmtApiSyncCmds::controlTraceLogging(void) {
  if (!FLAGS_enable_trace_dump) {
    return;
  }
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  // Trace control input params
  std::array<char, sizeof(device_mgmt_api::trace_control_e)> input_buff;
  device_mgmt_api::trace_control_e control = device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE;
  if (FLAGS_reset_trace_buffer) {
    control |= device_mgmt_api::TRACE_CONTROL_RESET_TRACEBUF;
  }

  memcpy(input_buff.data(), &control, sizeof(control));

  std::array<char, sizeof(uint8_t)> set_output_buff = {0};
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff.data(),
                                input_buff.size(), set_output_buff.data(), set_output_buff.size(), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

static inline void logTraceException(std::stringstream& logs, const struct trace_entry_header_t* entry) {
  const trace_execution_stack_t* tracePacketExecStack = templ::bit_cast<trace_execution_stack_t*>(entry);
  logs << "\nsepc = 0x" << std::hex << tracePacketExecStack->registers.epc << std::endl;
  logs << "stval = 0x" << std::hex << tracePacketExecStack->registers.tval << std::endl;
  logs << "sstatus = 0x" << std::hex << tracePacketExecStack->registers.status << std::endl;
  logs << "scause = 0x" << std::hex << tracePacketExecStack->registers.cause << std::endl;
  /* Log x1-x31 */
  for (int idx = 0; idx < TRACE_DEV_CONTEXT_GPRS; idx++) {
    logs << "x" << std::dec << idx + 1 << " = "
         << "0x" << std::hex << tracePacketExecStack->registers.gpr[idx] << std::endl;
  }
}

static inline bool decodeSingleTraceEvent(std::stringstream& logs, const struct trace_entry_header_t* entry) {
  auto validTypeFound = true;
  logs << "H:" << entry->hart_id << " Timestamp:" << entry->cycle << " :";
  if (entry->type == TRACE_TYPE_STRING) {
    std::array<char, TRACE_STRING_MAX_SIZE + 1> stringLog;
    const trace_string_t* tracePacketString = templ::bit_cast<trace_string_t*>(entry);
    snprintf(stringLog.data(), TRACE_STRING_MAX_SIZE + 1, "%s", tracePacketString->string);
    logs << stringLog.data() << std::endl;
  } else if (entry->type == TRACE_TYPE_EXCEPTION) {
    logTraceException(logs, entry);
  } else if (entry->type > TRACE_TYPE_STRING && entry->type <= TRACE_TYPE_CUSTOM_EVENT) {
    logs << "Trace Packet Type:" << entry->type
         << ", Use trace-utils decoder on trace binary file to parse this packet." << std::endl;
  } else {
    logs << "Invalid Trace Packet Type:" << entry->type << std::endl;
    validTypeFound = false;
  }
  return validTypeFound;
}

void TestDevMgmtApiSyncCmds::dumpRawTraceBuffer(int deviceIdx, const std::vector<std::byte>& traceBufRaw,
                                                TraceBufferType bufferType) const {
  if (traceBufRaw.empty()) {
    DV_LOG(INFO) << "Invalid trace buffer! size is 0";
    return;
  }
  std::vector<std::byte> traceBuf;
  traceBuf.insert(traceBuf.begin(), traceBufRaw.begin(), traceBufRaw.end());
  struct trace_buffer_std_header_t* traceHdr;
  std::string fileName = (fs::path(FLAGS_trace_bin_dir) / fs::path("dev" + std::to_string(deviceIdx) + "_")).string();
  unsigned int dataSize = 0;
  auto fileFlags = std::ofstream::binary;
  // Select the bin file
  switch (bufferType) {
  case TraceBufferType::TraceBufferSP:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "sp_";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferSPStats:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "sp_stats_";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferMM:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "mm_";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferMMStats:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "mm_stats_";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferCM:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->sub_buffer_count * traceHdr->sub_buffer_size;
    fileName += "cmsmode_";
    fileFlags |= std::ofstream::trunc;
    break;

  default:
    DV_LOG(INFO) << "Cannot dump unknown buffer type!";
    return;
  }

  fileName += std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) + ".bin";

  if (dataSize < sizeof(trace_buffer_std_header_t)) {
    return;
  }

  std::ofstream rawTrace;

  // Open the file
  rawTrace.open(fileName, fileFlags);
  if (rawTrace.is_open()) {
    bool update_size = false;

    // Check if the file is empty
    if (rawTrace.tellp() <= 0) {
      rawTrace.write(templ::bit_cast<char*>(traceHdr), sizeof(trace_buffer_std_header_t));
    } else {
      update_size = true;
    }
    // Remove the size of std header
    dataSize -= static_cast<unsigned int>(sizeof(trace_buffer_std_header_t));
    // Dump raw Trace data in the file (without std header)
    rawTrace.write(templ::bit_cast<char*>(traceBuf.data() + sizeof(trace_buffer_std_header_t)), dataSize);
    // Close the file
    rawTrace.close();

    // If we are appending data into existing file then update data size in raw trace header.
    if (update_size && dataSize) {
      unsigned int rawSize = 0;
      std::fstream tracefile(fileName, std::fstream::binary | std::fstream::in | std::fstream::out);

      // Get data existing data size in raw binary
      tracefile.seekg(offsetof(trace_buffer_std_header_t, data_size), std::fstream::beg);
      tracefile.read(templ::bit_cast<char*>(&rawSize), sizeof(traceHdr->data_size));

      // Update data size
      rawSize += dataSize;
      tracefile.seekp(offsetof(trace_buffer_std_header_t, data_size), std::fstream::beg);
      tracefile.write(templ::bit_cast<char*>(&rawSize), sizeof(traceHdr->data_size));
      tracefile.close();
    }
  } else {
    DV_LOG(INFO) << "Unable to open file: " << fileName;
  }
}

bool TestDevMgmtApiSyncCmds::decodeTraceEvents(int deviceIdx, const std::vector<std::byte>& traceBufRaw,
                                               TraceBufferType bufferType) const {
  if (traceBufRaw.empty()) {
    DV_LOG(INFO) << "Invalid trace buffer! size is 0";
    return false;
  }
  std::vector<std::byte> traceBuf;
  std::ofstream logfile;
  std::string fileName = getTraceTxtName(deviceIdx);
  traceBuf.insert(traceBuf.begin(), traceBufRaw.begin(), traceBufRaw.end());

  DV_LOG(INFO) << "Saving trace to file: " << fileName;
  logfile.open(fileName, std::ios_base::app);
  switch (bufferType) {
  case TraceBufferType::TraceBufferSP:
    logfile << "-> SP Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferSPStats:
    logfile << "-> SP Stats Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferMM:
    logfile << "-> MM S-Mode Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferMMStats:
    logfile << "-> MM Stats Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferCM:
    logfile << "-> CM S-Mode Traces" << std::endl;
    break;
  default:
    DV_LOG(INFO) << "Cannot decode unknown buffer type!";
    logfile.close();
    return false;
  }

  const struct trace_entry_header_t* entry = NULL;
  bool validEventFound = false;
  std::stringstream logs;
  for (entry = Trace_Decode(templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data()), entry); entry;
       entry = Trace_Decode(templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data()), entry)) {
    if (decodeSingleTraceEvent(logs, entry)) {
      validEventFound = true;
    }
  }

  logfile << logs.str();
  logfile.close();

  return validEventFound;
}

bool TestDevMgmtApiSyncCmds::extractAndPrintTraceData(bool singleDevice, TraceBufferType bufferType) {
  if (!FLAGS_enable_trace_dump) {
    return false;
  }

  getDM_t dmi = getInstance();
  EXPECT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::vector<std::byte> response;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  auto validTraceDataFound = false;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    if (dm.getTraceBufferServiceProcessor(deviceIdx, bufferType, response) != device_mgmt_api::DM_STATUS_SUCCESS) {
      DV_LOG(INFO) << "Unable to get trace buffer for device: " << deviceIdx << ". Disabling Trace.";
      continue;
    }
    dumpRawTraceBuffer(deviceIdx, response, bufferType);
    if (decodeTraceEvents(deviceIdx, response, bufferType)) {
      validTraceDataFound = true;
    }
  }

  return validTraceDataFound;
}

void TestDevMgmtApiSyncCmds::getModuleManufactureName(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperanto", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);
      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;
      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePartNumber(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = 4;
  char expected[output_size] = {0x63, 0x56, 0x34, 0x11};
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleSerialNumber(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = 4;
  unsigned char expected[output_size] = {0x78, 0x56, 0x34, 0x12};
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(memcmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICChipRevision(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePCIENumPortsMaxSpeed(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemorySizeMB(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = 1;
  char expected[output_size] = {16};
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleRevision(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = 4;
  unsigned char expected[output_size] = {0x44, 0xab, 0x11, 0x22};
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(memcmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleFormFactor(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = 1;
  char expected[output_size] = {1};
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemoryVendorPartNumber(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ((output_size > 0), 1);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemoryType(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      EXPECT_EQ(strncmp(asset_info->asset, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePowerState(bool singleDevice) {
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
    if (getTestTarget() != Target::Loopback) {
      uint8_t powerstate = get_output_buff[0];
      // Note: Module's Power State could vary. So there cannot be expected value for Power State in the test
      printf("Module's Power State: %d\n", powerstate);
    }
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagement(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

#define DM_TDP_LEVEL 25

void TestDevMgmtApiSyncCmds::setAndGetModuleStaticTDPLevel(bool singleDevice) {
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
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(uint8_t);
    char get_output_buff[get_output_size] = {0};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      uint8_t tdp_level = get_output_buff[0];
      EXPECT_EQ(tdp_level, DM_TDP_LEVEL);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModuleTemperatureThreshold(bool singleDevice) {
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
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::temperature_threshold_t);
    char get_output_buff[get_output_size] = {0};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS, nullptr,
                                0, get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::temperature_threshold_t* temperature_threshold =
        (device_mgmt_api::temperature_threshold_t*)get_output_buff;
      EXPECT_EQ(temperature_threshold->sw_temperature_c, 56);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyThrottleState(bool singleDevice) {
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
      const char input_buff[input_size] = {(char)throttle_state};
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);
      DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

      // Skip printing if loopback driver
      if (getTestTarget() != Target::Loopback) {
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

void TestDevMgmtApiSyncCmds::getModuleUptime(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::module_uptime_t* module_uptime = (device_mgmt_api::module_uptime_t*)output_buff;
      printf("Module uptime (day:hours:mins): %d:%d:%d\r\n", module_uptime->day, module_uptime->hours,
             module_uptime->mins);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModulePower(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Note: Module power could vary. So there cannot be expected value for Module power in the test
    device_mgmt_api::module_power_t* module_power = (device_mgmt_api::module_power_t*)output_buff;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      auto power = (module_power->power >> 2) + (module_power->power & 0x03) * 0.25;
      printf("Module power (in Watts): %.3f \n", power);

      EXPECT_NE(module_power->power, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleVoltage(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      // Note: Module power could vary. So there cannot be expected value for Module power in the test
      device_mgmt_api::module_voltage_t* module_voltage = (device_mgmt_api::module_voltage_t*)output_buff;

      voltage = 250 + (module_voltage->minion * 5);
      printf("Minion Shire Module Voltage (in millivolts): %d\n", voltage);

      EXPECT_NE(module_voltage->minion, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleCurrentTemperature(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::current_temperature_t* cur_temp = (device_mgmt_api::current_temperature_t*)output_buff;

      printf(" PMIC SYS current temperature (in C): %d\r\n", cur_temp->pmic_sys);
      printf(" IOSHIRE current temperature (in C): %d\r\n", cur_temp->ioshire_current);
      printf(" IOSHIRE low temperature (in C): %d\r\n", cur_temp->ioshire_low);
      printf(" IOSHIRE high temperature (in C): %d\r\n", cur_temp->ioshire_high);
      printf(" MINSHIRE average temperature (in C): %d\r\n", cur_temp->minshire_avg);
      printf(" MINSHIRE low temperature (in C): %d\r\n", cur_temp->minshire_low);
      printf(" MINSHIRE high temperature (in C): %d\r\n", cur_temp->minshire_high);

      EXPECT_NE(cur_temp->pmic_sys, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxTemperature(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::max_temperature_t* max_temperature = (device_mgmt_api::max_temperature_t*)output_buff;

      // Note: Module's Max Temperature could vary. So there cannot be expected value for max temperature in the test
      printf("Module's Max Temperature: %d\n", max_temperature->max_temperature_c);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxMemoryErrors(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::max_ecc_count_t* max_ecc_count = (device_mgmt_api::max_ecc_count_t*)output_buff;

      // Note: ECC count could vary. So there cannot be expected value for max_ecc_count in the test
      printf("Max ECC Count: %d\n", max_ecc_count->count);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMaxDDRBW(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyPowerState(bool singleDevice) {
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
      const char input_buff[input_size] = {(char)power_state};
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);
      DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

      // Skip printing if loopback driver
      if (getTestTarget() != Target::Loopback) {
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

void TestDevMgmtApiSyncCmds::setModuleFrequency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    for (device_mgmt_api::pll_id_e pll_id = device_mgmt_api::PLL_ID_NOC_PLL;
         pll_id <= device_mgmt_api::PLL_ID_MINION_PLL; pll_id++) {
      for (device_mgmt_api::use_step_e use_step = device_mgmt_api::USE_STEP_CLOCK_FALSE;
           use_step <= device_mgmt_api::USE_STEP_CLOCK_TRUE; use_step++) {
        if (use_step == device_mgmt_api::USE_STEP_CLOCK_TRUE && pll_id != device_mgmt_api::PLL_ID_MINION_PLL)
          continue;
        const uint32_t input_size =
          sizeof(uint16_t) + sizeof(device_mgmt_api::pll_id_e) + sizeof(device_mgmt_api::use_step_e);
        char input_buff[input_size];
        input_buff[0] = (char)(0x90);
        input_buff[1] = (char)(0x01);
        input_buff[2] = (char)pll_id;
        input_buff[3] = (char)use_step;
        auto hst_latency = std::make_unique<uint32_t>();
        auto dev_latency = std::make_unique<uint64_t>();

        EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_FREQUENCY, input_buff, input_size,
                                    nullptr, 0, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                  device_mgmt_api::DM_STATUS_SUCCESS);
        DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
      }
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetDDRECCThresholdCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetSRAMECCThresholdCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetPCIEECCThresholdCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getPCIEECCUECCCount(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getDDRECCUECCCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getSRAMECCUECCCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      device_mgmt_api::errors_count_t* errors_count = (device_mgmt_api::errors_count_t*)output_buff;

      EXPECT_EQ(errors_count->ecc, 0);
      EXPECT_EQ(errors_count->uecc, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getDDRBWCounter(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELinkSpeed(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIELaneWidth(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIERetrainPhy(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICFrequencies(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMBW(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilization(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilization(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICUtilization(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICStalls(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICLatency(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getMMErrorCount(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getFWBootstatus(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(output_buff[0], 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleFWRevision(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
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

void TestDevMgmtApiSyncCmds::serializeAccessMgmtNode(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Creating 100 threads and they all try to perform serviceRequest() after a single point of sync
    const auto totalThreads = 100;
    std::array<int, totalThreads> results;
    std::promise<void> syncPromise;
    std::shared_future syncFuture(syncPromise.get_future());

    auto testSerial = [&](uint32_t timeout, int* result) {
      const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();
      syncFuture.wait();
      *result = dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                  output_buff, output_size, hst_latency.get(), dev_latency.get(), timeout);
    };

    std::vector<std::thread> threads;
    for (auto threadId = 0; threadId < totalThreads; threadId++) {
      threads.push_back(
        std::thread(testSerial, (uint32_t)DM_SERVICE_REQUEST_TIMEOUT * totalThreads, &results[threadId]));
    }

    // Delay to ensure all threads started and reached the same point of sync
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    syncPromise.set_value();

    // Wait for threads completion
    for (auto& thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }

    for (auto threadId = 0; threadId < totalThreads; threadId++) {
      EXPECT_EQ(results[threadId], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::getDeviceErrorEvents(bool singleDevice) {
  int fd, i;
  char buff[BUFSIZ];
  ssize_t size = 0;
  int result = 0;
  const int max_err_types = 11;
  std::string line;
  std::string err_types[max_err_types] = {
    "PCIe Correctable Error", "PCIe Un-Correctable Error", "DRAM Correctable Error",     "DRAM Un-Correctable Error",
    "SRAM Correctable Error", "SRAM Un-Correctable Error", "Power Management IC Errors", "Minion Runtime Error",
    "Minion Runtime Hang",    "SP Runtime Error",          "SP Runtime Exception"};

  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    int err_count[max_err_types] = {0};
    result = 0;
    size = 0;
    fd = open("/dev/kmsg", (O_RDONLY | O_NONBLOCK));
    ASSERT_GE(fd, 0) << "Unable to read dmesg\n";
    ASSERT_NE(lseek(fd, 0, SEEK_END), -1) << "Unable to lseek() dmesg end\n";
    DV_LOG(INFO) << "waiting for error events...\n";

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DEVICE_ERROR_EVENTS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() == Target::Loopback) {
      close(fd);
      return;
    }

    EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Response received from device, wait for printing\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    DV_LOG(INFO) << "waiting done, starting events verification...\n";

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
      DV_LOG(INFO) << "matched '" << err_types[i] << "' " << err_count[i] << " time(s)\n";
      if (err_count[i] > 0) {
        result++;
      }
      EXPECT_GE(err_count[i], 1);
    }

    close(fd);

    // all events should match once
    EXPECT_EQ(result, max_err_types);
  }
}

void TestDevMgmtApiSyncCmds::isUnsupportedService(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Requests Completed for Device: " << deviceIdx;
  }
}

#define SP_CRT_512_V002 "../include/hash.txt"

void TestDevMgmtApiSyncCmds::setSpRootCertificate(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      char expected[output_size] = {0};
      strncpy(expected, "0", output_size);

      EXPECT_EQ(strncmp(output_buff, expected, output_size), 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::setTraceControl(bool singleDevice, uint32_t control_bitmap) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::trace_control_e);
    const char input_buff[input_size] = {(char)control_bitmap};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

void TestDevMgmtApiSyncCmds::setTraceConfigure(bool singleDevice, uint32_t event_mask, uint32_t filter_mask) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size =
      sizeof(device_mgmt_api::trace_configure_e) + sizeof(device_mgmt_api::trace_configure_filter_mask_e);
    const uint32_t input_buff[input_size] = {event_mask, filter_mask};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG,
                                reinterpret_cast<const char*>(input_buff), input_size, set_output_buff, set_output_size,
                                hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }
  }
}

// Note this test should always be the last one
void TestDevMgmtApiSyncCmds::getTraceBuffer(bool singleDevice, TraceBufferType bufferType) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  std::vector<std::byte> response;
  bool validEventFound = false;
  const struct trace_entry_header_t* entry = NULL;

  // Skip validation if loopback driver
  if (getTestTarget() == Target::Loopback) {
    DV_LOG(INFO) << "Get Trace Buffer is not supported on loopback driver";
    return;
  }

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    EXPECT_EQ(dm.getTraceBufferServiceProcessor(deviceIdx, bufferType, response), device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

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

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRange(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleSetTemperatureThresholdRange(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleStaticTDPLevelRange(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputSize(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputSize(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceNode(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidHostLatency(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceLatency(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputBuffer(bool singleDevice) {
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
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputBuffer(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::updateFirmwareImage(bool singleDevice) {
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

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      ASSERT_EQ(output_buff[0], 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIELinkSpeedToInvalidLinkSpeed(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_link_speed_e);
    const char input_buff[input_size] = {54}; // Invalid link speed

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED, input_buff,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELaneWidthToInvalidLaneWidth(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_lane_w_split_e);
    const char input_buff[input_size] = {86}; // Invalid lane width

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidOutputSize(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = INVALID_OUTPUT_SIZE; /*Invalid output size*/

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[OUTPUT_SIZE_TEST] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidDeviceNode(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    int device_node = deviceIdx + MAX_DEVICE_NODE; /*Invalid device node*/
    EXPECT_EQ(dm.serviceRequest(device_node, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidHostLatency(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size,
                                nullptr /*nullptr for invalid testing*/, dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidDeviceLatency(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                nullptr /*nullptr for invalid testing*/, DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidOutputBuffer(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char* output_buff = nullptr; /*nullptr for invalid testing*/
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getASICFrequenciesInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICFrequenciesInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICFrequenciesInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICFrequenciesInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICFrequenciesInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMBandwidthInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMBandwidthInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMBandwidthInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMBandwidthInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMBandwidthInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilizationInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilizationInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilizationInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilizationInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilizationInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilizationInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilizationInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilizationInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilizationInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilizationInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICUtilizationInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICUtilizationInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICUtilizationInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICUtilizationInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICUtilizationInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICStallsInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICStallsInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICStallsInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICStallsInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICStallsInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICLatencyInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICLatencyInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICLatencyInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICLatencyInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, singleDevice);
}

void TestDevMgmtApiSyncCmds::getASICLatencyInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, singleDevice);
}

void TestDevMgmtApiSyncCmds::testInvalidCmdCode(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, DM_CMD_INVALID, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidInputBuffer(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = INPUT_SIZE_TEST;
    const char* input_buff = nullptr; /*nullptr for invalid testing*/

    const uint32_t output_size = OUTPUT_SIZE_TEST;
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, input_buff, input_size, output_buff, output_size,
                                hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidInputSize(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = INVALID_INPUT_SIZE;
    const char input_buff[INPUT_SIZE_TEST] = {0};

    const uint32_t output_size = OUTPUT_SIZE_TEST;
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, input_buff, input_size, output_buff, output_size,
                                hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidInputBuffer(bool singleDevice) {
  testInvalidInputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidInputSize(bool singleDevice) {
  testInvalidInputSize(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setDDRECCCountInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCCountInvalidInputBuffer(bool singleDevice) {
  testInvalidInputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidInputSize(bool singleDevice) {
  testInvalidInputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setPCIEECCountInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCCountInvalidInputBuffer(bool singleDevice) {
  testInvalidInputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCCountInvalidInputSize(bool singleDevice) {
  testInvalidInputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCountInvalidOutputSize(bool singleDevice) {
  testInvalidOutputSize(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCountInvalidDeviceNode(bool singleDevice) {
  testInvalidDeviceNode(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCountInvalidHostLatency(bool singleDevice) {
  testInvalidHostLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCountInvalidDeviceLatency(bool singleDevice) {
  testInvalidDeviceLatency(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::setSRAMECCountInvalidOutputBuffer(bool singleDevice) {
  testInvalidOutputBuffer(device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT, singleDevice);
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidDeviceNode(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);
  char expected[output_size] = {0};

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx + deviceCount, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidHostLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, nullptr, dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidDeviceLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr, DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidOutputBuffer(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0, nullptr,
                                output_size, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidOutputSize(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0,
                                output_buff, 0, hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setThrottlePowerStatus(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  bool validEventFound = false;
  std::vector<std::byte> response;
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();
  const uint32_t output_size = sizeof(uint32_t);
  char output_buff[output_size] = {0};
  const struct trace_entry_header_t* entry = NULL;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    uint32_t input_size = sizeof(device_mgmt_api::power_throttle_state_e);
    for (device_mgmt_api::power_throttle_state_e throttle_state = device_mgmt_api::POWER_THROTTLE_STATE_POWER_DOWN;
         throttle_state >= device_mgmt_api::POWER_THROTTLE_STATE_POWER_UP; throttle_state--) {
      char input_buff[input_size] = {(char)throttle_state};
      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_THROTTLE_POWER_STATE_TEST, input_buff,
                                  input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                  DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    }
    input_size = sizeof(device_mgmt_api::trace_control_e);
    char input_buff_[input_size] = {device_mgmt_api::TRACE_CONTROL_TRACE_DISABLE};
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff_,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    if (getTestTarget() != Target::Loopback) {
      if (dm.getTraceBufferServiceProcessor(deviceIdx, TraceBufferType::TraceBufferSP, response) !=
          device_mgmt_api::DM_STATUS_SUCCESS) {
        DV_LOG(INFO) << "Unable to get SP trace buffer for device: " << deviceIdx << ". Disabling Trace.";
      } else {
        while (entry = Trace_Decode(reinterpret_cast<struct trace_buffer_std_header_t*>(response.data()), entry)) {
          if (entry->type == TRACE_TYPE_POWER_STATUS) {
            validEventFound = true;
            break;
          }
        }
      }
      EXPECT_TRUE(validEventFound) << "No SP trace event found!" << std::endl;
      validEventFound = false;
    }
    input_buff_[input_size] = {device_mgmt_api::TRACE_CONTROL_RESET_TRACEBUF |
                               device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE};

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff_,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
  }
}
void TestDevMgmtApiSyncCmds::resetMM(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    EXPECT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_MM_RESET, nullptr, 0, nullptr, 0, hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::resetMMOpsOpen(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    try {
      dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_MM_RESET, nullptr, 0, nullptr, 0, hst_latency.get(),
                        dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT);

    } catch (const dev::Exception& ex) {
      // Find and compare the if the error message is correct. 
      // Resetting of MM is not permitted when Ops node is open.
      ASSERT_NE(std::string(ex.what()).find("Operation not permitted"), std::string::npos);
    }
  }
}

void TestDevMgmtApiSyncCmds::readMem(uint64_t readAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  const uint32_t input_size = sizeof(device_mgmt_api::mdi_mem_read_t);
  device_mgmt_api::mdi_mem_read_t input_buff;
  /* Test address in HOST_MANAGED_DRAM_START - HOST_MANAGED_DRAM_END address range */
  input_buff.address = readAddr;
  input_buff.size = sizeof(uint64_t);
  const uint32_t output_size = sizeof(uint64_t);

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    uint64_t output = 0;
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM, (char*)&input_buff, input_size,
                                (char*)&output, output_size, hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "Mem addr: 0x" << std::hex << input_buff.address << " Value:" << std::hex << output;
    }

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::writeMem(uint64_t testInputData, uint64_t writeAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    const uint32_t mdi_mem_write_cmd_size = sizeof(device_mgmt_api::mdi_mem_write_t);
    device_mgmt_api::mdi_mem_write_t mdi_mem_write;
    mdi_mem_write.address = writeAddr;
    mdi_mem_write.size = sizeof(uint64_t);
    mdi_mem_write.data = testInputData;
    uint64_t mem_write_status = 0;

    DV_LOG(INFO) << "Mem addr: 0x" << std::hex << mdi_mem_write.address << " Write Value:" << std::hex
                 << mdi_mem_write.data;

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_MEM, (char*)&mdi_mem_write,
                                mdi_mem_write_cmd_size, (char*)&mem_write_status, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(mem_write_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t mdi_mem_read_cmd_size = sizeof(device_mgmt_api::mdi_mem_read_t);
    device_mgmt_api::mdi_mem_read_t mdi_mem_read;
    mdi_mem_read.address = writeAddr;
    mdi_mem_read.size = sizeof(uint64_t);
    uint64_t mem_read_output = 0;

    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM, (char*)&mdi_mem_read,
                                mdi_mem_read_cmd_size, (char*)&mem_read_output, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Mem addr: 0x" << std::hex << mdi_mem_read.address << " Read Value:" << std::hex << mem_read_output;

    EXPECT_EQ(mem_read_output, testInputData);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testRunControlCmdsSetandUnsetBreakpoint(uint64_t shireID, uint64_t threadMask,
                                                                     uint64_t hartID, uint64_t bpAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    // Set the Shire ID and Neigh ID.
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Set Breakpoint */
    const uint32_t mdi_bp_input_size = sizeof(device_mgmt_api::mdi_bp_control_t);
    device_mgmt_api::mdi_bp_control_t mdi_bp_input_buff;

    // Set the Hart ID/BP address/mode
    mdi_bp_input_buff.hart_id = hartID;
    mdi_bp_input_buff.bp_address = bpAddr;
    mdi_bp_input_buff.mode = device_mgmt_api::PRIV_MASK_PRIV_UMODE;
    int32_t bp_cmd_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SET_BREAKPOINT,
                                (char*)&mdi_bp_input_buff, mdi_bp_input_size, (char*)&bp_cmd_status, sizeof(int32_t),
                                hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Set BP at address: 0x" << std::hex << mdi_bp_input_buff.bp_address << " Status:" << std::hex
                 << bp_cmd_status;
    EXPECT_EQ(bp_cmd_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* UnSet Breakpoint */
    mdi_bp_input_buff = {0};
    mdi_bp_input_buff.hart_id = hartID;
    bp_cmd_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSET_BREAKPOINT,
                                (char*)&mdi_bp_input_buff, mdi_bp_input_size, (char*)&bp_cmd_status, sizeof(int32_t),
                                hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(bp_cmd_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testRunControlCmdsGetHartStatus(uint64_t shireID, uint64_t threadMask, uint64_t hartID) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    // Set the Shire ID and Neigh ID.
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Get the Hart Status */
    device_mgmt_api::mdi_hart_control_t mdi_hart_control_input;
    mdi_hart_control_input.hart_id = hartID;
    uint32_t hart_status;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_GET_HART_STATUS,
                                (char*)&mdi_hart_control_input, sizeof(mdi_hart_control_input), (char*)&hart_status,
                                sizeof(uint32_t), hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << mdi_hart_control_input.hart_id << " Status:" << std::hex << hart_status;
    }

    EXPECT_EQ(hart_status, device_mgmt_api::MDI_HART_STATUS_HALTED);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Get the Hart Status */
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_GET_HART_STATUS,
                                (char*)&mdi_hart_control_input, sizeof(mdi_hart_control_input), (char*)&hart_status,
                                sizeof(uint32_t), hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << mdi_hart_control_input.hart_id << " Status:" << std::hex << hart_status;
    }

    EXPECT_EQ(hart_status, device_mgmt_api::MDI_HART_STATUS_RUNNING);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testStateInspectionReadGPR(uint64_t shireID, uint64_t threadMask, uint64_t hartID) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    // Set the Shire ID and Neigh ID.
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t gpr_read_input_size = sizeof(device_mgmt_api::mdi_gpr_read_t);
    device_mgmt_api::mdi_gpr_read_t gpr_read_input_buff;
    gpr_read_input_buff.hart_id = hartID;
    /* Read starting from GPR index 1 */
    for (int i = 1; i < 32; i++) {
      uint64_t output = 0;
      gpr_read_input_buff.gpr_index = i;
      const uint32_t read_output_size = sizeof(uint64_t);
      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&output, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      if (getTestTarget() != Target::Loopback) {
        DV_LOG(INFO) << "HartID:" << std::hex << gpr_read_input_buff.hart_id
                     << " GPR Index:" << gpr_read_input_buff.gpr_index << " GPR REG Value:" << std::hex << output;
      }
    }

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testStateInspectionWriteGPR(uint64_t shireID, uint64_t threadMask, uint64_t hartID,
                                                         uint64_t writeTestData) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    // Set the Shire ID and Neigh ID.
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t gpr_read_input_size = sizeof(device_mgmt_api::mdi_gpr_read_t);
    device_mgmt_api::mdi_gpr_read_t gpr_read_input_buff;
    gpr_read_input_buff.hart_id = hartID;

    const uint32_t gpr_write_input_size = sizeof(device_mgmt_api::mdi_gpr_write_t);
    device_mgmt_api::mdi_gpr_write_t gpr_write_input_buff;
    uint64_t dummy;
    gpr_write_input_buff.hart_id = hartID;

    uint64_t output = 0;
    const uint32_t read_output_size = sizeof(uint64_t);
    for (int i = 1; i < 32; i++) {
      gpr_read_input_buff.gpr_index = i;
      gpr_write_input_buff.gpr_index = i;

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&output, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "Before update HartID:" << std::hex << gpr_read_input_buff.hart_id
                   << " GPR Index:" << gpr_read_input_buff.gpr_index << " Value:" << std::hex << output;

      gpr_write_input_buff.data = writeTestData + i; /* Test Data */
      DV_LOG(INFO) << "Write Value:" << std::hex << gpr_write_input_buff.data;
      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_GPR,
                                  (char*)&gpr_write_input_buff, gpr_write_input_size, (char*)&dummy, sizeof(uint64_t),
                                  hst_latency.get(), dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&output, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "After update HartID:" << std::hex << gpr_read_input_buff.hart_id
                   << " GPR Index:" << gpr_read_input_buff.gpr_index << " Value:" << std::hex << output;

      EXPECT_EQ(gpr_write_input_buff.data, output);
    }

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testStateInspectionReadCSR(uint64_t shireID, uint64_t threadMask, uint64_t hartID,
                                                        uint64_t csrName) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    // Set the Shire ID and Neigh ID.
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t csr_read_input_size = sizeof(device_mgmt_api::mdi_csr_read_t);
    device_mgmt_api::mdi_csr_read_t csr_read_input_buff;
    csr_read_input_buff.hart_id = hartID;
    /* PC offset */
    csr_read_input_buff.csr_name = csrName;
    uint64_t csr_value = 0;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&csr_value, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " PC Value:" << std::hex << csr_value;
    }

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testStateInspectionWriteCSR(uint64_t shireID, uint64_t threadMask, uint64_t hartID,
                                                         uint64_t csrName, uint64_t csrData) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    /* Select Hart */
    const uint32_t mdi_hart_select_input_size = sizeof(device_mgmt_api::mdi_hart_selection_t);
    device_mgmt_api::mdi_hart_selection_t mdi_hart_select_input_buff;
    mdi_hart_select_input_buff.shire_id = shireID;
    mdi_hart_select_input_buff.thread_mask = threadMask;
    int32_t hart_select_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t csr_read_input_size = sizeof(device_mgmt_api::mdi_csr_read_t);
    device_mgmt_api::mdi_csr_read_t csr_read_input_buff;
    csr_read_input_buff.hart_id = hartID;
    /* PC offset */
    csr_read_input_buff.csr_name = csrName;
    uint64_t intial_pc_addr = 0;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&intial_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Initial PC Value:" << std::hex
                 << intial_pc_addr;

    const uint32_t csr_write_input_size = sizeof(device_mgmt_api::mdi_csr_write_t);
    device_mgmt_api::mdi_csr_write_t csr_write_input_buff;
    csr_write_input_buff.hart_id = hartID;
    csr_write_input_buff.csr_name = csrName;
    csr_write_input_buff.data = csrData;
    uint64_t dummy = 0;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_CSR, (char*)&csr_write_input_buff,
                                csr_write_input_size, (char*)&dummy, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    uint64_t updated_pc_addr = 0;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&updated_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Updated PC Value:" << std::hex
                 << updated_pc_addr;

    EXPECT_EQ(csr_write_input_buff.data, updated_pc_addr);

    /* Reset the PC to initial PC address */
    csr_write_input_buff.data = intial_pc_addr; /* Reset it to initial PC address */
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_CSR, (char*)&csr_write_input_buff,
                                csr_write_input_size, (char*)&dummy, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    updated_pc_addr = 0;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&updated_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Changed PC to initial value:" << std::hex
                 << updated_pc_addr;

    EXPECT_EQ(intial_pc_addr, updated_pc_addr);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    EXPECT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DM_SERVICE_REQUEST_TIMEOUT),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}
