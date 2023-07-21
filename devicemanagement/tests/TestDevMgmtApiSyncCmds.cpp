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
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <unistd.h>

namespace fs = std::experimental::filesystem;

using namespace dev;
using namespace device_management;
using namespace std::chrono_literals;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

#define DURATION2MS(dur) (((dur).count() > 0) ? std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() : 0)
#define BIN2VOLTAGE(REG_VALUE, BASE, MULTIPLIER, DIVIDER) (BASE + ((REG_VALUE * MULTIPLIER) / DIVIDER))
#define VOLTAGE2BIN(VOL_VALUE, BASE, MULTIPLIER, DIVIDER) (uint8_t)(((VOL_VALUE - BASE) * DIVIDER) / MULTIPLIER)
#define POWER_10MW_TO_MW(pwr_10mw) (pwr_10mw * 10)
#define POWER_10MW_TO_W(pwr_10mw) (pwr_10mw / 100)

DEFINE_bool(enable_trace_dump, true,
            "Enable SP trace dump to file specified by flag: trace_logfile, otherwise on UART");
DEFINE_bool(reset_trace_buffer, true,
            "Reset the SP trace buffer on the start of the test run if trace logging is enabled");
DEFINE_string(trace_base_dir, "devtrace", "Base directory which will contain all traces");
DEFINE_string(trace_txt_dir, FLAGS_trace_base_dir + "/txt_files",
              "A directory in the current path where the decoded device traces will be printed");
DEFINE_string(trace_bin_dir, FLAGS_trace_base_dir + "/bin_files",
              "A directory in the current path where the raw device traces will be dumped");
DEFINE_uint32(exec_timeout_ms, 30000, "Internal execution timeout in milliseconds");

#define FORMAT_VERSION(major, minor, revision) ((major << 24) | (minor << 16) | (revision << 8))
#define MAJOR_VERSION(ver) ((ver >> 24) & 0xff)
#define MINOR_VERSION(ver) ((ver >> 16) & 0xff)
#define REVISION_VERSION(ver) ((ver >> 8) & 0xff)

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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff.data(),
                                input_buff.size(), set_output_buff.data(), set_output_buff.size(), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::dmStatsRunControl(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto setStatsRunControl = [&](int deviceIdx, device_mgmt_api::stats_type_e type,
                                device_mgmt_api::stats_control_e control) {
    // Trace control input params
    std::array<char, sizeof(type) + sizeof(control)> input_buff;
    memcpy(input_buff.data(), &type, sizeof(type));
    memcpy(input_buff.data() + sizeof(type), &control, sizeof(control));
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_STATS_RUN_CONTROL, input_buff.data(),
                          input_buff.size(), nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    return true;
  };

  auto findStatsSampleInStatsBuffer = [&](int deviceIdx, TraceBufferType type) {
    std::vector<std::byte> buff;
    bool found = false;
    if (dm.getTraceBufferServiceProcessor(deviceIdx, type, buff) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return found;
    } else {
      for (const trace_entry_header_t* entry = nullptr;
           entry = Trace_Decode(static_cast<trace_buffer_std_header_t*>(static_cast<void*>(buff.data())), entry);) {
        if (entry->type == TRACE_TYPE_CUSTOM_EVENT) {
          found = true;
          break;
        }
      }
    }
    return found;
  };

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Disable the trace logging
    ASSERT_TRUE(setStatsRunControl(deviceIdx, device_mgmt_api::STATS_TYPE_SP | device_mgmt_api::STATS_TYPE_MM,
                                   device_mgmt_api::STATS_CONTROL_TRACE_DISABLE))
      << fmt::format("Device[{}]: setStatsRunControl() failed!", deviceIdx);

    // Reset the trace buffer
    ASSERT_TRUE(setStatsRunControl(deviceIdx, device_mgmt_api::STATS_TYPE_SP | device_mgmt_api::STATS_TYPE_MM,
                                   device_mgmt_api::STATS_CONTROL_TRACE_DISABLE |
                                     device_mgmt_api::STATS_CONTROL_RESET_COUNTER |
                                     device_mgmt_api::STATS_CONTROL_RESET_TRACEBUF))
      << fmt::format("Device[{}]: setStatsRunControl() failed!", deviceIdx);
    EXPECT_FALSE(findStatsSampleInStatsBuffer(deviceIdx, TraceBufferType::TraceBufferSPStats))
      << fmt::format("Device[{}]: No SP Stats should have received!", deviceIdx);
    EXPECT_FALSE(findStatsSampleInStatsBuffer(deviceIdx, TraceBufferType::TraceBufferMMStats))
      << fmt::format("Device[{}]: No MM Stats should have received!", deviceIdx);

    ASSERT_TRUE(setStatsRunControl(deviceIdx, device_mgmt_api::STATS_TYPE_SP | device_mgmt_api::STATS_TYPE_MM,
                                   device_mgmt_api::STATS_CONTROL_TRACE_ENABLE))
      << fmt::format("Device[{}]: setStatsRunControl() failed!", deviceIdx);
  }

  std::vector<bool> spStatsFound(deviceCount, false);
  std::vector<bool> mmStatsFound(deviceCount, false);
  auto loopDelay = (getTestTarget() == Target::Silicon) ? std::chrono::milliseconds(250) : std::chrono::seconds(1);
  // Stats not immediately available, requires a delay. Wait until SP and MM Stats of all devices are received.
  for (; Clock::now() < end && !(std::all_of(spStatsFound.begin(), spStatsFound.end(), [](bool v) { return v; }) &&
                                 std::all_of(mmStatsFound.begin(), mmStatsFound.end(), [](bool v) { return v; }));
       std::this_thread::sleep_for(loopDelay)) {
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      if (!spStatsFound[deviceIdx]) {
        spStatsFound[deviceIdx] = findStatsSampleInStatsBuffer(deviceIdx, TraceBufferType::TraceBufferSPStats);
      }
      if (!mmStatsFound[deviceIdx]) {
        mmStatsFound[deviceIdx] = findStatsSampleInStatsBuffer(deviceIdx, TraceBufferType::TraceBufferMMStats);
      }
    }
  }
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    EXPECT_TRUE(spStatsFound[deviceIdx]) << fmt::format("Device[{}]: SP stats should have received now!", deviceIdx);
    EXPECT_TRUE(mmStatsFound[deviceIdx]) << fmt::format("Device[{}]: MM stats should have received now!", deviceIdx);
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperanto", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
  uint32_t partNumber;
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER, nullptr, 0,
                                static_cast<char*>(static_cast<void*>(&partNumber)), sizeof(partNumber),
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received partNumber={:08x}", deviceIdx, partNumber);
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModulePartNumber(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto setModulePartNumber = [&](int deviceIdx, uint32_t partNumber) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting partNumber={:08x}", deviceIdx, partNumber);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_PART_NUMBER,
                          static_cast<char*>(static_cast<void*>(&partNumber)), sizeof(partNumber), nullptr, 0,
                          hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    return true;
  };

  auto getModulePartNumber = [&](int deviceIdx) -> std::optional<uint32_t> {
    uint32_t partNumber;
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER, nullptr, 0,
                          static_cast<char*>(static_cast<void*>(&partNumber)), sizeof(partNumber), hst_latency.get(),
                          dev_latency.get(), DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return std::nullopt;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received partNumber={:08x}", deviceIdx, partNumber);
    return partNumber;
  };

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Get default partNumber
    auto container = getModulePartNumber(deviceIdx);
    ASSERT_TRUE(container.has_value()) << "getModulePartNumber() failed!";
    auto defaultPartNumber = container.value();

    // Set the testPartNumber (0xdeadbeef)
    uint32_t testPartNumber = 0xdeadbeef;
    ASSERT_TRUE(setModulePartNumber(deviceIdx, testPartNumber)) << "setModulePartNumber() failed!";

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      // Validate testPartNumber
      container = getModulePartNumber(deviceIdx);
      ASSERT_TRUE(container.has_value()) << "getModulePartNumber() failed!";
      EXPECT_EQ(container.value(), testPartNumber) << "Unable to set test partNumber";
    }

    // Revert back to default partNumber
    ASSERT_TRUE(setModulePartNumber(deviceIdx, defaultPartNumber)) << "setModulePartNumber() failed";

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      // Validate default partNumber is set
      container = getModulePartNumber(deviceIdx);
      ASSERT_TRUE(container.has_value()) << "getModulePartNumber() failed!";
      EXPECT_EQ(container.value(), defaultPartNumber) << "Unable to revert to default partNumber";
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleSerialNumber(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  ecid_t ecid;
  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      memcpy(&ecid, output_buff, output_size);
      printf("  Lot ID       = %s (0x%016lx)\n", ecid.lot_id_str, ecid.lot_id);
      printf("  Wafer ID     = 0x%02x (%d)\n", ecid.wafer_id, ecid.wafer_id);
      printf("  X Coordinate = 0x%02x (%d)\n", ecid.x_coordinate, ecid.x_coordinate);
      printf("  Y Coordinate = 0x%02x (%d)\n", ecid.y_coordinate, ecid.y_coordinate);
    }
  }
}

void TestDevMgmtApiSyncCmds::getASICChipRevision(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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

void TestDevMgmtApiSyncCmds::getModulePCIEPortsMaxSpeed(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::asset_info_t* asset_info = (device_mgmt_api::asset_info_t*)output_buff;

      // TODO: SW-13272: Compare the max link speed with max_link_speed sysfs attribute
      // auto sysfsMaxLinkSpeed = devLayer_->getDeviceAttribute(deviceIdx, "max_link_speed");
      // auto receivedMaxLinkSpeed = std::string(asset_info->asset);
      // EXPECT_NE(sysfsMaxLinkSpeed.find(receivedMaxLinkSpeed), std::string::npos)
      //   << "Max Link Speed: received: " << receivedMaxLinkSpeed << " vs sysfs_attribute: " << sysfsMaxLinkSpeed;
      EXPECT_TRUE(strncmp(asset_info->asset, "16", output_size) == 0 ||
                  strncmp(asset_info->asset, "8", output_size) == 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleMemorySizeMB(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = 1;
  std::vector<std::array<uint8_t, output_size>> expected = {{16}, {32}};

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::array<uint8_t, output_size> output_buff = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB, nullptr, 0,
                                static_cast<char*>(static_cast<void*>(output_buff.data())), output_buff.size(),
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_TRUE(std::any_of(expected.begin(), expected.end(),
                              [&](const std::array<uint8_t, output_size>& value) {
                                return std::memcmp(output_buff.data(), value.data(), output_buff.size()) == 0;
                              }))
        << fmt::format("Expected: {} whereas output: {}", fmt::join(expected, ", "), output_buff);
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleRevision(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = 4;
  unsigned char expected[output_size] = {0x44, 0xab, 0x11, 0x22};
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = 1;
  char expected[output_size] = {1};
  printf("expected: %.*s\n", output_size, expected);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    const uint32_t get_output_size = sizeof(device_mgmt_api::power_state_e);
    char get_output_buff[get_output_size] = {0};

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER_STATE, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    char input_buff[input_size] = {device_mgmt_api::ACTIVE_POWER_MANAGEMENT_TURN_ON};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    // The active power management is disabled by default, so disabling it to revert back to default state
    input_buff[input_size] = {device_mgmt_api::ACTIVE_POWER_MANAGEMENT_TURN_OFF};
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  uint8_t cur_tdp_level;
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(uint8_t);
    char input_buff[input_size] = {DM_TDP_LEVEL};
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    const uint32_t get_output_size = sizeof(uint8_t);
    char get_output_buff[get_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    /* save current TDP level*/
    cur_tdp_level = static_cast<int>(get_output_buff[0]);

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ((uint32_t)set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL, nullptr, 0,
                                get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      uint8_t tdp_level = get_output_buff[0];
      EXPECT_EQ(tdp_level, DM_TDP_LEVEL);
    }

    /* Restore TDP level */
    input_buff[0] = (char)cur_tdp_level;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModuleTemperatureThreshold(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::temperature_threshold_t);
    const char input_buff[input_size] = {(uint8_t)56};

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      EXPECT_EQ(set_output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);
    }

    const uint32_t get_output_size = sizeof(device_mgmt_api::temperature_threshold_t);
    char get_output_buff[get_output_size] = {0};

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS, nullptr,
                                0, get_output_buff, get_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
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

      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::module_uptime_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_UPTIME, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::module_power_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver or SysEMU
    if (!targetInList({Target::Loopback, Target::SysEMU})) {
      // Note: Module power could vary. So there cannot be expected value for Module power in the test
      device_mgmt_api::module_power_t* module_power = (device_mgmt_api::module_power_t*)output_buff;
      float power = POWER_10MW_TO_W((float)module_power->power);
      DV_LOG(INFO) << "Module power (in Watts): \n" << power;

      EXPECT_NE(module_power->power, 0);
    }
  }
}

void TestDevMgmtApiSyncCmds::getAsicVoltage(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    device_mgmt_api::asic_voltage_t* moduleVoltage;
    const uint32_t output_size = sizeof(device_mgmt_api::asic_voltage_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_VOLTAGE, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    device_mgmt_api::asic_voltage_t* voltages = (device_mgmt_api::asic_voltage_t*)output_buff;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received DDR={} mV", deviceIdx, BIN2VOLTAGE(voltages->ddr, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received L2CACHE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->l2_cache, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MAXION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->maxion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MINION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->minion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received PShire={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->pshire_0p75, 600, 125, 10));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received NOC={} mV", deviceIdx, BIN2VOLTAGE(voltages->noc, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received IOShire={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->ioshire_0p75, 600, 625, 100));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQ={} mV", deviceIdx, BIN2VOLTAGE(voltages->vddq, 250, 10, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQLP={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->vddqlp, 250, 10, 1));
    // Skip validation if loopback driver or SysEMU
    if (!targetInList({Target::Loopback, Target::SysEMU})) {
      // Expect that output_buff is non-zero
      EXPECT_TRUE(
        std::any_of(output_buff, output_buff + output_size, [](unsigned char const byte) { return byte != 0; }));
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleVoltage(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    device_mgmt_api::module_voltage_t* moduleVoltage;
    const uint32_t output_size = sizeof(device_mgmt_api::module_voltage_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    device_mgmt_api::module_voltage_t* voltages = (device_mgmt_api::module_voltage_t*)output_buff;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received DDR={} mV", deviceIdx, BIN2VOLTAGE(voltages->ddr, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received L2CACHE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->l2_cache, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MAXION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->maxion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MINION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->minion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received PCIE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->pcie, 600, 125, 10));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received NOC={} mV", deviceIdx, BIN2VOLTAGE(voltages->noc, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received PCIE_LOGIC={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->pcie_logic, 600, 625, 100));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQ={} mV", deviceIdx, BIN2VOLTAGE(voltages->vddq, 250, 10, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQLP={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->vddqlp, 250, 10, 1));
    // Skip validation if loopback driver or SysEMU
    if (!targetInList({Target::Loopback, Target::SysEMU})) {
      // Expect that output_buff is non-zero
      EXPECT_TRUE(
        std::any_of(output_buff, output_buff + output_size, [](unsigned char const byte) { return byte != 0; }));
    }
  }
}

void TestDevMgmtApiSyncCmds::setAndGetModuleVoltage(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
  double voltagePercentDrop = 0.0;

  auto setModuleVoltages = [&](int deviceIdx, const device_mgmt_api::module_voltage_t& voltages) {
    const uint32_t input_size = sizeof(device_mgmt_api::module_e) + sizeof(uint8_t);
    char input_buff[input_size];
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting DDR={} mV", deviceIdx, BIN2VOLTAGE(voltages.ddr, 250, 5, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_DDR;
    input_buff[1] = (char)(voltages.ddr);
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting L2CACHE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages.l2_cache, 250, 5, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_L2CACHE;
    input_buff[1] = (char)(voltages.l2_cache);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting MAXION={} mV", deviceIdx, BIN2VOLTAGE(voltages.maxion, 250, 5, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_MAXION;
    input_buff[1] = (char)(voltages.maxion);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting MINION={} mV", deviceIdx, BIN2VOLTAGE(voltages.minion, 250, 5, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_MINION;
    input_buff[1] = (char)(voltages.minion);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting PCIE={} mV", deviceIdx, BIN2VOLTAGE(voltages.pcie, 600, 125, 10));
    input_buff[0] = (char)device_mgmt_api::MODULE_PCIE;
    input_buff[1] = (char)(voltages.pcie);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting NOC={} mV", deviceIdx, BIN2VOLTAGE(voltages.noc, 250, 5, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_NOC;
    input_buff[1] = (char)(voltages.noc);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting PCIE_LOGIC={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages.pcie_logic, 600, 625, 100));
    input_buff[0] = (char)device_mgmt_api::MODULE_PCIE_LOGIC;
    input_buff[1] = (char)(voltages.pcie_logic);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting VDDQ={} mV", deviceIdx, BIN2VOLTAGE(voltages.vddq, 250, 10, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_VDDQ;
    input_buff[1] = (char)(voltages.vddq);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting VDDQLP={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages.vddqlp, 250, 10, 1));
    input_buff[0] = (char)device_mgmt_api::MODULE_VDDQLP;
    input_buff[1] = (char)(voltages.vddqlp);
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_VOLTAGE, input_buff, input_size,
                          nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    return true;
  };

  auto getModuleVoltages = [&](int deviceIdx) -> std::optional<device_mgmt_api::module_voltage_t> {
    device_mgmt_api::module_voltage_t* moduleVoltage;
    const uint32_t output_size = sizeof(device_mgmt_api::module_voltage_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE, nullptr, 0, output_buff,
                          output_size, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return std::nullopt;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    device_mgmt_api::module_voltage_t* voltages = (device_mgmt_api::module_voltage_t*)output_buff;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received DDR={} mV", deviceIdx, BIN2VOLTAGE(voltages->ddr, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received L2CACHE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->l2_cache, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MAXION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->maxion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received MINION={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->minion, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received PCIE={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->pcie, 600, 125, 10));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received NOC={} mV", deviceIdx, BIN2VOLTAGE(voltages->noc, 250, 5, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received PCIE_LOGIC={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->pcie_logic, 600, 625, 100));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQ={} mV", deviceIdx, BIN2VOLTAGE(voltages->vddq, 250, 10, 1));
    DV_LOG(INFO) << fmt::format("Device[{}]: Received VDDQLP={} mV", deviceIdx,
                                BIN2VOLTAGE(voltages->vddqlp, 250, 10, 1));

    return *voltages;
  };

  auto percentVoltageChange = [](uint8_t newBinVoltage, uint8_t refBinVoltage, uint32_t base, uint32_t multiplier,
                                 uint32_t divider) {
    auto newVoltage = static_cast<double>(BIN2VOLTAGE(newBinVoltage, base, multiplier, divider));
    auto refVoltage = static_cast<double>(BIN2VOLTAGE(refBinVoltage, base, multiplier, divider));
    return std::abs(newVoltage - refVoltage) / refVoltage;
  };

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Get default voltages
    auto container = getModuleVoltages(deviceIdx);
    ASSERT_TRUE(container.has_value()) << "getModuleVoltages() failed!";
    auto defaultVoltages = container.value();

    // Set test voltages (5 steps (25mV/30mV) greater than default)
    device_mgmt_api::module_voltage_t testVoltages;
    testVoltages.ddr = defaultVoltages.ddr + 5;
    testVoltages.l2_cache = defaultVoltages.l2_cache + 5;
    testVoltages.maxion = defaultVoltages.maxion + 5;
    testVoltages.minion = defaultVoltages.minion + 5;
    testVoltages.pcie = defaultVoltages.pcie + 5;
    testVoltages.noc = defaultVoltages.noc + 5;
    testVoltages.pcie_logic = defaultVoltages.pcie_logic + 5;
    testVoltages.vddq = defaultVoltages.vddq + 1;
    testVoltages.vddqlp = defaultVoltages.vddqlp + 1;
    ASSERT_TRUE(setModuleVoltages(deviceIdx, testVoltages)) << "setModuleVoltages() failed!";

    // Skip validation if loopback driver or SysEMU
    if (!targetInList({Target::Loopback, Target::SysEMU})) {
      for (auto retry = 0; retry < 3; retry++) {
        // Wait for voltages to settle
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Validate test voltages
        container = getModuleVoltages(deviceIdx);
        ASSERT_TRUE(container.has_value()) << "getModuleVoltages() failed!";
        if (memcmp(&defaultVoltages, &container.value(), sizeof(defaultVoltages)) != 0) {
          break;
        }
      }
      EXPECT_LE(percentVoltageChange(container.value().ddr, testVoltages.ddr, 250, 5, 1), voltagePercentDrop)
        << "Unable to set DDR test voltage";
      EXPECT_LE(percentVoltageChange(container.value().l2_cache, testVoltages.l2_cache, 250, 5, 1), voltagePercentDrop)
        << "Unable to set L2CACHE test voltage";
      EXPECT_LE(percentVoltageChange(container.value().maxion, testVoltages.maxion, 250, 5, 1), voltagePercentDrop)
        << "Unable to set MAXION test voltage";
      EXPECT_LE(percentVoltageChange(container.value().minion, testVoltages.minion, 250, 5, 1), voltagePercentDrop)
        << "Unable to set MINION test voltage";
      EXPECT_LE(percentVoltageChange(container.value().pcie, testVoltages.pcie, 600, 125, 10), voltagePercentDrop)
        << "Unable to set PCIE test voltage";
      EXPECT_LE(percentVoltageChange(container.value().noc, testVoltages.noc, 250, 5, 1), voltagePercentDrop)
        << "Unable to set NOC test voltage";
      EXPECT_LE(percentVoltageChange(container.value().pcie_logic, testVoltages.pcie_logic, 600, 625, 100),
                voltagePercentDrop)
        << "Unable to set PCIE_LOGIC test voltage";
      EXPECT_LE(percentVoltageChange(container.value().vddq, testVoltages.vddq, 250, 10, 1), voltagePercentDrop)
        << "Unable to set VDDQ test voltage";
      EXPECT_LE(percentVoltageChange(container.value().vddqlp, testVoltages.vddqlp, 250, 10, 1), voltagePercentDrop)
        << "Unable to set VDDQLP test voltage";
      EXPECT_NE(memcmp(&defaultVoltages, &container.value(), sizeof(defaultVoltages)), 0)
        << "No changes in voltage after SET_MODULE_VOLTAGE command";
    }

    // Revert back to original voltages
    ASSERT_TRUE(setModuleVoltages(deviceIdx, defaultVoltages)) << "setModuleVoltages() failed";

    // Skip validation if loopback driver or SysEMU
    if (!targetInList({Target::Loopback, Target::SysEMU})) {
      for (auto retry = 0; retry < 3; retry++) {
        // Wait for voltages to settle
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Validate default voltages are set
        container = getModuleVoltages(deviceIdx);
        ASSERT_TRUE(container.has_value()) << "getModuleVoltages() failed!";
        if (memcmp(&testVoltages, &container.value(), sizeof(testVoltages)) != 0) {
          break;
        }
      }
      EXPECT_LE(percentVoltageChange(container.value().ddr, defaultVoltages.ddr, 250, 5, 1), voltagePercentDrop)
        << "Unable to set DDR default voltage";
      EXPECT_LE(percentVoltageChange(container.value().l2_cache, defaultVoltages.l2_cache, 250, 5, 1),
                voltagePercentDrop)
        << "Unable to set L2CACHE default voltage";
      EXPECT_LE(percentVoltageChange(container.value().maxion, defaultVoltages.maxion, 250, 5, 1), voltagePercentDrop)
        << "Unable to set MAXION default voltage";
      EXPECT_LE(percentVoltageChange(container.value().minion, defaultVoltages.minion, 250, 5, 1), voltagePercentDrop)
        << "Unable to set MINION default voltage";
      EXPECT_LE(percentVoltageChange(container.value().pcie, defaultVoltages.pcie, 600, 125, 10), voltagePercentDrop)
        << "Unable to set PCIE default voltage";
      EXPECT_LE(percentVoltageChange(container.value().noc, defaultVoltages.noc, 250, 5, 1), voltagePercentDrop)
        << "Unable to set NOC default voltage";
      EXPECT_LE(percentVoltageChange(container.value().pcie_logic, defaultVoltages.pcie_logic, 600, 625, 100),
                voltagePercentDrop)
        << "Unable to set PCIE_LOGIC default voltage";
      EXPECT_LE(percentVoltageChange(container.value().vddq, defaultVoltages.vddq, 250, 10, 1), voltagePercentDrop)
        << "Unable to set VDDQ test voltage";
      EXPECT_LE(percentVoltageChange(container.value().vddqlp, defaultVoltages.vddqlp, 250, 10, 1), voltagePercentDrop)
        << "Unable to set VDDQLP test voltage";
      EXPECT_NE(memcmp(&testVoltages, &container.value(), sizeof(testVoltages)), 0)
        << "No changes in voltage after SET_MODULE_VOLTAGE command";
    }
  }
}

void TestDevMgmtApiSyncCmds::getModuleCurrentTemperature(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::current_temperature_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::max_temperature_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::max_ecc_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::max_dram_bw_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleResidencyPowerState(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
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

      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES,
                                  input_buff, input_size, output_buff, output_size, hst_latency.get(),
                                  dev_latency.get(), DURATION2MS(end - Clock::now())),
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

void TestDevMgmtApiSyncCmds::setAndGetModuleFrequency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto setMinionAndNocFrequencies = [&](int deviceIdx,
                                        const std::pair<uint16_t /* minionFreq */, uint16_t /* nocFreq */>& freqs) {
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting Minion={}, NOC={} MHz Frequencies.", deviceIdx, freqs.first,
                                freqs.second);
    for (device_mgmt_api::pll_id_e pll_id = device_mgmt_api::PLL_ID_NOC_PLL;
         pll_id <= device_mgmt_api::PLL_ID_MINION_PLL; pll_id++) {
      uint16_t pll_freq = (pll_id == device_mgmt_api::PLL_ID_NOC_PLL) ? freqs.second : freqs.first;
      const uint32_t input_size =
        sizeof(uint16_t) + sizeof(device_mgmt_api::pll_id_e) + sizeof(device_mgmt_api::use_step_e);
      char input_buff[input_size];
      input_buff[0] = (char)(pll_freq & 0xff);
      input_buff[1] = (char)((pll_freq >> 8) & 0xff);
      input_buff[2] = (char)pll_id;
      input_buff[3] = (char)device_mgmt_api::USE_STEP_CLOCK_TRUE;
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();
      if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_FREQUENCY, input_buff, input_size, nullptr,
                            0, hst_latency.get(), dev_latency.get(),
                            DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
        return false;
      }
      DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    }
    return true;
  };

  auto getMinionAndNocFrequencies = [&](int deviceIdx) -> std::optional<std::pair<uint16_t, uint16_t>> {
    const uint32_t output_size = sizeof(device_mgmt_api::asic_frequencies_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, nullptr, 0, output_buff,
                          output_size, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return std::nullopt;
    }
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    auto* asic_frequencies = (device_mgmt_api::asic_frequencies_t*)output_buff;
    DV_LOG(INFO) << fmt::format("Device[{}]: Received Frequencies: Minion={}, NOC={} MHz.", deviceIdx,
                                asic_frequencies->minion_shire_mhz, asic_frequencies->noc_mhz);
    return std::make_pair<uint16_t, uint16_t>(asic_frequencies->minion_shire_mhz, asic_frequencies->noc_mhz);
  };

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Get current frequencies
    auto origFreqs = getMinionAndNocFrequencies(deviceIdx);
    ASSERT_TRUE(origFreqs.has_value()) << "Unable to get current Minion and NOC Frequencies!";

    std::vector<std::pair<uint16_t, uint16_t>> testFreqsList = {{400, 200}, {500, 250}, {600, 300}};
    for (auto testFreqs : testFreqsList) {
      // Set test frequencies
      ASSERT_TRUE(setMinionAndNocFrequencies(deviceIdx, testFreqs))
        << fmt::format("Unable to set Minion={} MHz and NOC={} MHz Frequencies!", testFreqs.first, testFreqs.second);
      auto freqs = getMinionAndNocFrequencies(deviceIdx);
      ASSERT_TRUE(freqs.has_value()) << "Unable to get test Minion and NOC Frequencies!";
      EXPECT_EQ(freqs.value().first, testFreqs.first)
        << fmt::format("Couldn't set test Minion={} MHz Frequency!", testFreqs.first);
      EXPECT_EQ(freqs.value().second, testFreqs.second)
        << fmt::format("Couldn't set test NOC={} Frequency!", testFreqs.second);
    }

    // Revert back to original frequencies
    ASSERT_TRUE(setMinionAndNocFrequencies(deviceIdx, origFreqs.value())) << fmt::format(
      "Unable to set Minion={} MHz and NOC={} MHz Frequencies!", origFreqs.value().first, origFreqs.value().second);
    auto freqs = getMinionAndNocFrequencies(deviceIdx);
    ASSERT_TRUE(freqs.has_value()) << "Unable to get current Minion and NOC Frequencies!";
    EXPECT_EQ(freqs.value().first, origFreqs.value().first)
      << fmt::format("Couldn't revert to original Minion={} Frequency!", origFreqs.value().first);
    EXPECT_EQ(freqs.value().second, origFreqs.value().second)
      << fmt::format("Couldn't revert to original NOC={} Frequency!", origFreqs.value().second);
  }
}

void TestDevMgmtApiSyncCmds::setAndGetDDRECCThresholdCount(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::errors_count_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dram_bw_counter_t);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELinkSpeed(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto setLaneWidth = [&](int deviceIdx, const device_mgmt_api::pcie_lane_w_split_e laneWidth) {
    std::array<char, sizeof(device_mgmt_api::pcie_lane_w_split_e)> input_buff;
    memcpy(input_buff.data(), &laneWidth, input_buff.size());
    DV_LOG(INFO) << fmt::format("Device[{}]: Setting Lane Width x{}", deviceIdx,
                                laneWidth == device_mgmt_api::PCIE_LANE_W_SPLIT_x8 ? 8 : 4);
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    if (dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff.data(),
                          input_buff.size(), nullptr, 0, hst_latency.get(), dev_latency.get(),
                          DURATION2MS(end - Clock::now())) != device_mgmt_api::DM_STATUS_SUCCESS) {
      return false;
    }
    DV_LOG(INFO) << fmt::format("Service Request Completed for Device: {}", deviceIdx);
    return true;
  };

  auto getLaneWidth = [&](int deviceIdx) -> std::optional<device_mgmt_api::pcie_lane_w_split_e> {
    auto laneWidth =
      std::stoi(devLayer_->getDeviceAttribute(deviceIdx, "max_link_width")); // available for silicon only
    if (laneWidth == 4) {
      return device_mgmt_api::PCIE_LANE_W_SPLIT_x4;
    } else if (laneWidth == 8) {
      return device_mgmt_api::PCIE_LANE_W_SPLIT_x8;
    }
    return std::nullopt;
  };

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    device_mgmt_api::pcie_lane_w_split_e defaultLaneWidth;
    device_mgmt_api::pcie_lane_w_split_e testLaneWidth;
    if (getTestTarget() == Target::Silicon) {
      auto container = getLaneWidth(deviceIdx);
      if (!container.has_value()) {
        DV_LOG(INFO) << fmt::format("Device[{}]: The default lane width is not restorable, skipping the test",
                                    deviceIdx);
        continue;
      }
      defaultLaneWidth = container.value();
    } else {
      defaultLaneWidth = device_mgmt_api::PCIE_LANE_W_SPLIT_x4;
    }

    if (defaultLaneWidth == device_mgmt_api::PCIE_LANE_W_SPLIT_x4) {
      testLaneWidth = device_mgmt_api::PCIE_LANE_W_SPLIT_x8;
    } else {
      testLaneWidth = device_mgmt_api::PCIE_LANE_W_SPLIT_x4;
    }

    // Set test lane width
    ASSERT_TRUE(setLaneWidth(deviceIdx, testLaneWidth)) << fmt::format("Device[{}]: setLaneWidth() failed!", deviceIdx);
    // Validate on Target::Silicon only
    if (getTestTarget() == Target::Silicon) {
      ASSERT_EQ(testLaneWidth, getLaneWidth(deviceIdx).value())
        << fmt::format("Device[{}]: Failed to set testLaneWidth!", deviceIdx);
    }

    // Restore default lane width
    ASSERT_TRUE(setLaneWidth(deviceIdx, defaultLaneWidth))
      << fmt::format("Device[{}]: setLaneWidth() failed!", deviceIdx);
    // Validate on Target::Silicon only
    if (getTestTarget() == Target::Silicon) {
      ASSERT_EQ(defaultLaneWidth, getLaneWidth(deviceIdx).value())
        << fmt::format("Device[{}]: Failed to restore defaultLaneWidth!", deviceIdx);
    }
  }
}

void TestDevMgmtApiSyncCmds::setPCIERetrainPhy(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asic_frequencies_t);
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMBW(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::dram_bw_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getDRAMCapacityUtilization(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::percentage_cap_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getASICPerCoreDatapathUtilization(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Actual Payload is TBD. So device is currently returning the status of cmd execution
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::mm_error_count_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MM_ERROR_COUNT, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getFWBootstatus(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // DM_CMD_GET_FIRMWARE_BOOT_STATUS : Device returns response of type device_mgmt_default_rsp_t.
    // Payload in response is of type uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS, nullptr, 0, output_buff,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::firmware_version_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip printing and validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      printf("output_buff: %.*s\n", output_size, output_buff);

      device_mgmt_api::firmware_version_t* firmware_versions = (device_mgmt_api::firmware_version_t*)output_buff;

      std::stringstream ss;
      ss << "\nFirmware Versions:" << std::endl;
      ss << fmt::format("\tbl1_v:   {}.{}.{}", MAJOR_VERSION(firmware_versions->bl1_v),
                        MINOR_VERSION(firmware_versions->bl1_v), REVISION_VERSION(firmware_versions->bl1_v))
         << std::endl;
      ss << fmt::format("\tbl2_v:   {}.{}.{}", MAJOR_VERSION(firmware_versions->bl2_v),
                        MINOR_VERSION(firmware_versions->bl2_v), REVISION_VERSION(firmware_versions->bl2_v))
         << std::endl;
      ss << fmt::format("\tpmic_v: {}.{}.{}", MAJOR_VERSION(firmware_versions->pmic_v),
                        MINOR_VERSION(firmware_versions->pmic_v), REVISION_VERSION(firmware_versions->pmic_v))
         << std::endl;
      ss << fmt::format("\tmm_v:    {}.{}.{}", MAJOR_VERSION(firmware_versions->mm_v),
                        MINOR_VERSION(firmware_versions->mm_v), REVISION_VERSION(firmware_versions->mm_v))
         << std::endl;
      ss << fmt::format("\twm_v:    {}.{}.{}", MAJOR_VERSION(firmware_versions->wm_v),
                        MINOR_VERSION(firmware_versions->wm_v), REVISION_VERSION(firmware_versions->wm_v))
         << std::endl;
      ss << fmt::format("\tmachm_v: {}.{}.{}", MAJOR_VERSION(firmware_versions->machm_v),
                        MINOR_VERSION(firmware_versions->machm_v), REVISION_VERSION(firmware_versions->machm_v))
         << std::endl;
      DV_LOG(INFO) << ss.str();
    }
  }
}

void TestDevMgmtApiSyncCmds::serializeAccessMgmtNode(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Creating 100 threads and they all try to perform serviceRequest() after a single point of sync
    const auto totalThreads = 100;
    std::array<int, totalThreads> results;
    std::promise<void> syncPromise;
    std::shared_future syncFuture(syncPromise.get_future());

    auto testSerial = [&](int* result) {
      const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
      char output_buff[output_size] = {0};
      auto hst_latency = std::make_unique<uint32_t>();
      auto dev_latency = std::make_unique<uint64_t>();
      syncFuture.wait();
      *result = dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                  output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                  DURATION2MS(end - Clock::now()));
    };

    std::vector<std::thread> threads;
    for (auto threadId = 0; threadId < totalThreads; threadId++) {
      threads.push_back(std::thread(testSerial, &results[threadId]));
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
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
  std::vector<std::string> errTypes = {"DramCeEvent",        "MinionCeEvent",   "PcieCeEvent", "PmicCeEvent",
                                       "SpCeEvent",          "SpExceptCeEvent", "SramCeEvent", "DramUceEvent",
                                       "MinionHangUceEvent", "PcieUceEvent",    "SramUceEvent"};
  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<uint64_t> oldErrCount(errTypes.size(), 0);
    std::vector<uint64_t> newErrCount(errTypes.size(), 0);

    // Skip counters reading if loopback driver
    if (getTestTarget() != Target::Loopback) {
      // Read error statistics from sysfs counters
      auto errStats = devLayer_->getDeviceAttribute(deviceIdx, "err_stats/ce_count") +
                      devLayer_->getDeviceAttribute(deviceIdx, "err_stats/uce_count");
      for (auto typeIdx = 0; typeIdx < errTypes.size(); typeIdx++) {
        std::smatch match;
        std::regex rgx(errTypes[typeIdx] + ":\\s+(\\d+)");
        ASSERT_TRUE(std::regex_search(errStats, match, rgx)) << "" << errTypes[typeIdx] << "not found!";
        oldErrCount[typeIdx] = std::strtoull(match.str(1).c_str(), nullptr, 10);
      }
    }

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_DEVICE_ERROR_EVENTS, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Skip validation if loopback driver
    if (getTestTarget() == Target::Loopback) {
      continue;
    }

    EXPECT_EQ(output_buff[0], device_mgmt_api::DM_STATUS_SUCCESS);

    // Read error statistics again from sysfs counters
    auto errStats = devLayer_->getDeviceAttribute(deviceIdx, "err_stats/ce_count") +
                    devLayer_->getDeviceAttribute(deviceIdx, "err_stats/uce_count");
    for (auto typeIdx = 0; typeIdx < errTypes.size(); typeIdx++) {
      std::smatch match;
      std::regex rgx(errTypes[typeIdx] + ":\\s+(\\d+)");
      ASSERT_TRUE(std::regex_search(errStats, match, rgx)) << "" << errTypes[typeIdx] << "not found!";
      newErrCount[typeIdx] = std::strtoull(match.str(1).c_str(), nullptr, 10);

      // The counter value must be incremented by now
      EXPECT_GT(newErrCount[typeIdx], oldErrCount[typeIdx]) << errTypes[typeIdx] << " not received!";
      DV_LOG(INFO) << errTypes[typeIdx] << ": " << oldErrCount[typeIdx] << " -> " << newErrCount[typeIdx];
    }
  }
}

void TestDevMgmtApiSyncCmds::isUnsupportedService(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    // Invalid device node
    ASSERT_EQ(dm.serviceRequest(deviceIdx + 66, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);

    // Invalid command code
    ASSERT_EQ(dm.serviceRequest(deviceIdx, 99999, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    // Invalid input_buffer
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);

    // Invalid output_buffer
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0, nullptr,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    // Invalid host latency
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, nullptr, dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    // Invalid device latency
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr, DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Requests Completed for Device: " << deviceIdx;
  }
}

#define SP_CRT_512_V002 "../include/hash.txt"

void TestDevMgmtApiSyncCmds::setSpRootCertificate(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    // Payload in response is of type uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT, SP_CRT_512_V002, 1,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::trace_control_e);
    const char input_buff[input_size] = {(char)control_bitmap};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff,
                                input_size, set_output_buff, set_output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size =
      sizeof(device_mgmt_api::trace_configure_e) + sizeof(device_mgmt_api::trace_configure_filter_mask_e);
    const uint32_t input_buff[input_size] = {event_mask, filter_mask};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG,
                                reinterpret_cast<const char*>(input_buff), input_size, set_output_buff, set_output_size,
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const char input_buff[input_size] = {9}; // Invalid active power management

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleSetTemperatureThresholdRange(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::temperature_threshold_t);
    const char input_buff[input_size] = {(uint8_t)2, (uint8_t)80}; // Invalid temperature ranges

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t set_output_size = sizeof(uint32_t);
    char set_output_buff[set_output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                                input_buff, input_size, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputSize(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const char input_buff[input_size] = {device_mgmt_api::ACTIVE_POWER_MANAGEMENT_TURN_ON};

    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                input_buff, 0 /* Invalid size*/, set_output_buff, set_output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputSize(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, 0 /*Invalid output size*/, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceNode(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx + 250, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr,
                                0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidHostLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, nullptr /*nullptr for invalid testing*/, dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidDeviceLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr /*nullptr for invalid testing*/,
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getModuleManufactureNameInvalidOutputBuffer(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::asset_info_t);
  char expected[output_size] = {0};
  strncpy(expected, "Esperan", output_size);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME, nullptr, 0,
                                nullptr /*nullptr instaed of output buffer*/, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setModuleActivePowerManagementRangeInvalidInputBuffer(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::active_power_management_e);
    const uint32_t set_output_size = sizeof(uint8_t);
    char set_output_buff[set_output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT,
                                nullptr /*Nullptr instead of input buffer*/, input_size, set_output_buff,
                                set_output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::updateFirmwareImage(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_link_speed_e);
    const char input_buff[input_size] = {54}; // Invalid link speed

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED, input_buff,
                                input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::setPCIELaneWidthToInvalidLaneWidth(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = sizeof(device_mgmt_api::pcie_lane_w_split_e);
    const char input_buff[input_size] = {86}; // Invalid lane width

    // Device rsp will be of type device_mgmt_default_rsp_t and payload is uint32_t
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH, input_buff, input_size,
                                output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidOutputSize(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = INVALID_OUTPUT_SIZE; /*Invalid output size*/

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[OUTPUT_SIZE_TEST] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidDeviceNode(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    int device_node = deviceIdx + MAX_DEVICE_NODE; /*Invalid device node*/
    ASSERT_EQ(dm.serviceRequest(device_node, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidHostLatency(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size,
                                nullptr /*nullptr for invalid testing*/, dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidDeviceLatency(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                nullptr /*nullptr for invalid testing*/, DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidOutputBuffer(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char* output_buff = nullptr; /*nullptr for invalid testing*/
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = OUTPUT_SIZE_TEST;

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, DM_CMD_INVALID, nullptr, 0, output_buff, output_size, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidInputBuffer(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = INPUT_SIZE_TEST;
    const char* input_buff = nullptr; /*nullptr for invalid testing*/

    const uint32_t output_size = OUTPUT_SIZE_TEST;
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, input_buff, input_size, output_buff, output_size,
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testInvalidInputSize(int32_t dmCmdType, bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    const uint32_t input_size = INVALID_INPUT_SIZE;
    const char input_buff[INPUT_SIZE_TEST] = {0};

    const uint32_t output_size = OUTPUT_SIZE_TEST;
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, dmCmdType, input_buff, input_size, output_buff, output_size,
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);
  char expected[output_size] = {0};

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx + deviceCount, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE,
                                nullptr, 0, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidHostLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE, nullptr, 0,
                                output_buff, output_size, nullptr, dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidDeviceLatency(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW, nullptr, 0,
                                output_buff, output_size, hst_latency.get(), nullptr, DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidOutputBuffer(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0, nullptr,
                                output_size, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EINVAL);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::getHistoricalExtremeWithInvalidOutputSize(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t output_size = sizeof(device_mgmt_api::dm_cmd_e);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    char output_buff[output_size] = {0};
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR, nullptr, 0,
                                output_buff, 0, hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();
  const uint32_t output_size = sizeof(uint32_t);
  char output_buff[output_size] = {0};

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    uint32_t input_size = sizeof(device_mgmt_api::power_throttle_state_e);
    for (device_mgmt_api::power_throttle_state_e throttle_state = device_mgmt_api::POWER_THROTTLE_STATE_POWER_DOWN;
         throttle_state >= device_mgmt_api::POWER_THROTTLE_STATE_POWER_UP; throttle_state--) {
      char input_buff[input_size] = {(char)throttle_state};
      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_THROTTLE_POWER_STATE_TEST, input_buff,
                                  input_size, output_buff, output_size, hst_latency.get(), dev_latency.get(),
                                  DURATION2MS(end - Clock::now())),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    }

    if (getTestTarget() != Target::Loopback) {
      const struct trace_entry_header_t* entry = NULL;
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
      EXPECT_TRUE(validEventFound) << "No SP Power status trace event found for Device: " << deviceIdx << std::endl;
      validEventFound = false;
    }
  }
}
void TestDevMgmtApiSyncCmds::resetMM(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_MM_RESET, nullptr, 0, nullptr, 0, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::resetMMWithOpsInUse(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    try {
      dm.serviceRequest(0, device_mgmt_api::DM_CMD::DM_CMD_MM_RESET, nullptr, 0, nullptr, 0, hst_latency.get(),
                        dev_latency.get(), DURATION2MS(end - Clock::now()));

    } catch (const dev::Exception& ex) {
      // Resetting of MM is not permitted when Ops node is open.
      EXPECT_THAT(ex.what(), testing::HasSubstr("Operation not permitted"));
    }
  }
}

void TestDevMgmtApiSyncCmds::readMem_unprivileged(uint64_t readAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t input_size = sizeof(device_mgmt_api::mdi_mem_read_t);
  device_mgmt_api::mdi_mem_read_t input_buff;
  /* Test address in un-privileged memory regions. */
  input_buff.address = readAddr;
  input_buff.hart_id = 0;
  input_buff.access_type = 2; /* MEM_ACCESS_TYPE_NORMAL */
  input_buff.size = sizeof(uint64_t);
  const uint32_t output_size = sizeof(uint64_t);

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    uint64_t output = 0;
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM, (char*)&input_buff, input_size,
                                (char*)&output, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    // Skip validation if loopback driver
    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "Mem addr: 0x" << std::hex << input_buff.address << " Value:" << std::hex << output;
    }

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::readMem_privileged(uint64_t readAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  const uint32_t input_size = sizeof(device_mgmt_api::mdi_mem_read_t);
  device_mgmt_api::mdi_mem_read_t input_buff;
  /* Test address in privileged memory regions. */
  input_buff.address = readAddr;
  input_buff.hart_id = 0;
  input_buff.access_type = 2; /* MEM_ACCESS_TYPE_NORMAL */
  input_buff.size = sizeof(uint64_t);
  const uint32_t output_size = sizeof(uint64_t);

  auto deviceCount = dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    uint64_t output = 0;
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM, (char*)&input_buff, input_size,
                                (char*)&output, output_size, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              -EIO);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::writeMem_unprivileged(uint64_t testInputData, uint64_t writeAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_MEM, (char*)&mdi_mem_write,
                                mdi_mem_write_cmd_size, (char*)&mem_write_status, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(mem_write_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t mdi_mem_read_cmd_size = sizeof(device_mgmt_api::mdi_mem_read_t);
    device_mgmt_api::mdi_mem_read_t mdi_mem_read;
    mdi_mem_read.address = writeAddr;
    mdi_mem_read.hart_id = 0;
    mdi_mem_read.access_type = 2; /* MEM_ACCESS_TYPE_NORMAL */
    mdi_mem_read.size = sizeof(uint64_t);
    mdi_mem_read.hart_id = 0;
    mdi_mem_read.access_type = 2; /* MEM_ACCESS_TYPE_NORMAL */
    uint64_t mem_read_output = 0;

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM, (char*)&mdi_mem_read,
                                mdi_mem_read_cmd_size, (char*)&mem_read_output, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Mem addr: 0x" << std::hex << mdi_mem_read.address << " Read Value:" << std::hex << mem_read_output;

    EXPECT_EQ(mem_read_output, testInputData);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::writeMem_privileged(uint64_t testInputData, uint64_t writeAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_MEM, (char*)&mdi_mem_write,
                                mdi_mem_write_cmd_size, (char*)&mem_write_status, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              -EIO);

    EXPECT_EQ(mem_write_status, device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testRunControlCmdsSetandUnsetBreakpoint(uint64_t shireID, uint64_t threadMask,
                                                                     uint64_t hartID, uint64_t bpAddr) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SET_BREAKPOINT,
                                (char*)&mdi_bp_input_buff, mdi_bp_input_size, (char*)&bp_cmd_status, sizeof(int32_t),
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    /* Wait for set break point event */
    std::vector<std::byte> buf;
    EXPECT_TRUE(dm.getEvent(deviceIdx, buf, DURATION2MS(end - Clock::now())));

    auto rCB = reinterpret_cast<const dm_evt*>(buf.data());
    EXPECT_EQ(rCB->info.event_hdr.msg_id, device_mgmt_api::DM_CMD_MDI_SET_BREAKPOINT_EVENT);

    /* UnSet Breakpoint */
    mdi_bp_input_buff = {0};
    mdi_bp_input_buff.hart_id = hartID;
    bp_cmd_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSET_BREAKPOINT,
                                (char*)&mdi_bp_input_buff, mdi_bp_input_size, (char*)&bp_cmd_status, sizeof(int32_t),
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(bp_cmd_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testRunControlCmdsGetHartStatus(uint64_t shireID, uint64_t threadMask, uint64_t hartID) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Get the Hart Status */
    device_mgmt_api::mdi_hart_control_t mdi_hart_control_input;
    mdi_hart_control_input.hart_id = hartID;
    uint32_t hart_status;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_GET_HART_STATUS,
                                (char*)&mdi_hart_control_input, sizeof(mdi_hart_control_input), (char*)&hart_status,
                                sizeof(uint32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << mdi_hart_control_input.hart_id << " Status:" << std::hex << hart_status;
    }

    EXPECT_EQ(hart_status, device_mgmt_api::MDI_HART_STATUS_HALTED);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Get the Hart Status */
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_GET_HART_STATUS,
                                (char*)&mdi_hart_control_input, sizeof(mdi_hart_control_input), (char*)&hart_status,
                                sizeof(uint32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << mdi_hart_control_input.hart_id << " Status:" << std::hex << hart_status;
    }

    EXPECT_EQ(hart_status, device_mgmt_api::MDI_HART_STATUS_RUNNING);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::testStateInspectionReadGPR(uint64_t shireID, uint64_t threadMask, uint64_t hartID) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
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
      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&output, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DURATION2MS(end - Clock::now())),
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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
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
    uint64_t gpr_val = 0;
    const uint32_t read_output_size = sizeof(uint64_t);
    for (int i = 1; i < 32; i++) {
      gpr_read_input_buff.gpr_index = i;
      gpr_write_input_buff.gpr_index = i;

      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&gpr_val, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DURATION2MS(end - Clock::now())),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "Before update HartID:" << std::hex << gpr_read_input_buff.hart_id
                   << " GPR Index:" << gpr_read_input_buff.gpr_index << " Value:" << std::hex << gpr_val;

      gpr_write_input_buff.data = writeTestData + i; /* Test Data */
      DV_LOG(INFO) << "Write Value:" << std::hex << gpr_write_input_buff.data;
      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_GPR,
                                  (char*)&gpr_write_input_buff, gpr_write_input_size, (char*)&dummy, sizeof(uint64_t),
                                  hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
                device_mgmt_api::DM_STATUS_SUCCESS);

      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR, (char*)&gpr_read_input_buff,
                                  gpr_read_input_size, (char*)&output, read_output_size, hst_latency.get(),
                                  dev_latency.get(), DURATION2MS(end - Clock::now())),
                device_mgmt_api::DM_STATUS_SUCCESS);

      DV_LOG(INFO) << "After update HartID:" << std::hex << gpr_read_input_buff.hart_id
                   << " GPR Index:" << gpr_read_input_buff.gpr_index << " Value:" << std::hex << output;

      EXPECT_EQ(gpr_write_input_buff.data, output);

      /* Restore GPR value */
      gpr_write_input_buff.data = gpr_val;
      ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_GPR,
                                  (char*)&gpr_write_input_buff, gpr_write_input_size, (char*)&dummy, sizeof(uint64_t),
                                  hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
                device_mgmt_api::DM_STATUS_SUCCESS);
    }

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t csr_read_input_size = sizeof(device_mgmt_api::mdi_csr_read_t);
    device_mgmt_api::mdi_csr_read_t csr_read_input_buff;
    csr_read_input_buff.hart_id = hartID;
    /* PC offset */
    csr_read_input_buff.csr_name = csrName;
    uint64_t csr_value = 0;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&csr_value, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    if (getTestTarget() != Target::Loopback) {
      DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " PC Value:" << std::hex << csr_value;
    }

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
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
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

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
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&hart_select_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(hart_select_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Halt Hart */
    uint32_t hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    char hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_HALT_HART};
    int32_t halt_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&halt_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(halt_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    const uint32_t csr_read_input_size = sizeof(device_mgmt_api::mdi_csr_read_t);
    device_mgmt_api::mdi_csr_read_t csr_read_input_buff;
    csr_read_input_buff.hart_id = hartID;
    /* PC offset */
    csr_read_input_buff.csr_name = csrName;
    uint64_t intial_pc_addr = 0;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&intial_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Initial PC Value:" << std::hex
                 << intial_pc_addr;

    const uint32_t csr_write_input_size = sizeof(device_mgmt_api::mdi_csr_write_t);
    device_mgmt_api::mdi_csr_write_t csr_write_input_buff;
    csr_write_input_buff.hart_id = hartID;
    csr_write_input_buff.csr_name = csrName;
    csr_write_input_buff.data = csrData;
    uint64_t dummy = 0;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_CSR, (char*)&csr_write_input_buff,
                                csr_write_input_size, (char*)&dummy, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    uint64_t updated_pc_addr = 0;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&updated_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Updated PC Value:" << std::hex
                 << updated_pc_addr;

    EXPECT_EQ(csr_write_input_buff.data, updated_pc_addr);

    /* Reset the PC to initial PC address */
    csr_write_input_buff.data = intial_pc_addr; /* Reset it to initial PC address */
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_CSR, (char*)&csr_write_input_buff,
                                csr_write_input_size, (char*)&dummy, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    updated_pc_addr = 0;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR, (char*)&csr_read_input_buff,
                                csr_read_input_size, (char*)&updated_pc_addr, sizeof(uint64_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "HartID:" << std::hex << csr_read_input_buff.hart_id << " Changed PC to initial value:" << std::hex
                 << updated_pc_addr;

    EXPECT_EQ(intial_pc_addr, updated_pc_addr);

    /* Resume Hart */
    hart_control_input_size = sizeof(device_mgmt_api::mdi_hart_control_t);
    hart_control_input_buff[hart_control_input_size] = {device_mgmt_api::MDI_HART_CTRL_FLAG_RESUME_HART};
    int32_t resume_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART, hart_control_input_buff,
                                hart_control_input_size, (char*)&resume_hart_status, sizeof(int32_t), hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(resume_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    /* Unselect Hart */
    int32_t unselect_hart_status = device_mgmt_api::DM_STATUS_ERROR;
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART,
                                (char*)&mdi_hart_select_input_buff, mdi_hart_select_input_size,
                                (char*)&unselect_hart_status, sizeof(int32_t), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    EXPECT_EQ(unselect_hart_status, device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
  }
}

void TestDevMgmtApiSyncCmds::resetSOC(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_RESET_ETSOC, nullptr, 0, nullptr, 0,
                                hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Check if trace control works after reset
    controlTraceLogging();
  }
}

void TestDevMgmtApiSyncCmds::resetSOCWithOpsInUse(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();
    try {
      dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_RESET_ETSOC, nullptr, 0, nullptr, 0,
                        hst_latency.get(), dev_latency.get(), DURATION2MS(end - Clock::now()));
    } catch (const dev::Exception& ex) {
      // ETSOC reset is not permitted when Ops node is open.
      EXPECT_THAT(ex.what(), testing::HasSubstr("Operation not permitted"));
    }
  }
}

void TestDevMgmtApiSyncCmds::testShireCacheConfig(bool singleDevice) {
  getDM_t dmi = getInstance();
  ASSERT_TRUE(dmi);
  DeviceManagement& dm = (*dmi)(devLayer_.get());
  auto end = Clock::now() + std::chrono::milliseconds(FLAGS_exec_timeout_ms);
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  auto deviceCount = singleDevice ? 1 : dm.getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {

    /* Fetch shire cache config */
    device_mgmt_api::shire_cache_config_t sc_config = {0};
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_SHIRE_CACHE_CONFIG, nullptr, 0,
                                (char*)&sc_config, sizeof(sc_config), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);

    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;
    DV_LOG(INFO) << "SC config scp size: " << sc_config.scp_size << "  L2 size " << sc_config.l2_size << " L3 size "
                 << sc_config.l3_size;

    device_mgmt_api::shire_cache_config_t sc_config_modified = {64, 32, 32};
    DV_LOG(INFO) << "Modifying SC config to scp size: " << sc_config_modified.scp_size << "  L2 size "
                 << sc_config_modified.l2_size << " L3 size " << sc_config_modified.l3_size;

    /* Modify shire cache config*/
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SHIRE_CACHE_CONFIG,
                                (char*)&sc_config_modified, sizeof(sc_config_modified), nullptr, 0, hst_latency.get(),
                                dev_latency.get(), DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    /* Fetch shire cache config */
    device_mgmt_api::shire_cache_config_t tmp_sc_config = {0};
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_GET_SHIRE_CACHE_CONFIG, nullptr, 0,
                                (char*)&tmp_sc_config, sizeof(tmp_sc_config), hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    /* Validate if the configuration has changed */
    EXPECT_EQ(sc_config_modified.scp_size, tmp_sc_config.scp_size);
    EXPECT_EQ(sc_config_modified.l2_size, tmp_sc_config.l2_size);
    EXPECT_EQ(sc_config_modified.l3_size, tmp_sc_config.l3_size);

    /* Restore shire cache config values */
    ASSERT_EQ(dm.serviceRequest(deviceIdx, device_mgmt_api::DM_CMD::DM_CMD_SET_SHIRE_CACHE_CONFIG, (char*)&sc_config,
                                sizeof(sc_config), nullptr, 0, hst_latency.get(), dev_latency.get(),
                                DURATION2MS(end - Clock::now())),
              device_mgmt_api::DM_STATUS_SUCCESS);
    DV_LOG(INFO) << "Service Request Completed for Device: " << deviceIdx;

    // Check if trace control works after reset
    controlTraceLogging();
  }
}
