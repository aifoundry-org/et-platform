//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "deviceManagement/DeviceManagement.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <cerrno>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <device-layer/IDeviceLayer.h>
#include <dlfcn.h>
#include <exception>
#include <experimental/filesystem>
#include <fcntl.h>
#include <fstream>
#include <getopt.h>
#include <glog/logging.h>
#include <iostream>
#include <regex>
#include <string>
#include <unistd.h>
#define ET_TRACE_DECODER_IMPL
#include <esperanto/et-trace/decoder.h>
#include <hostUtils/logging/Logging.h>

#define DM_LOG(severity) ET_LOG(DEV_MNGT_SERVICE, severity) // severity levels: INFO, WARNING and FATAL respectively
#define DM_DLOG(severity) ET_DLOG(DEV_MNGT_SERVICE, severity)
#define DM_VLOG(level) ET_VLOG(DEV_MNGT_SERVICE, level) // severity levels: LOW MID HIGH
#define BIN2VOLTAGE(REG_VALUE, BASE, MULTIPLIER) (BASE + REG_VALUE * MULTIPLIER)
#define VOLTAGE2BIN(VOL_VALUE, BASE, MULTIPLIER) ((VOL_VALUE - BASE) / MULTIPLIER)

namespace fs = std::experimental::filesystem;

using namespace dev;
using namespace device_mgmt_api;
using namespace device_management;
using namespace std::chrono_literals;
using namespace boost::multiprecision;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

class DMLib {
public:
  DMLib() {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
  }

  ~DMLib() {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }

  getDM_t getInstance() {
    const char* error;

    if (handle_) {
      getDM_t getDM = reinterpret_cast<getDM_t>(dlsym(handle_, "getInstance"));
      if (!(error = dlerror())) {
        return getDM;
      }
      DM_VLOG(HIGH) << "error:" << error << std::endl;
    }
    return (getDM_t)0;
  }

  int verifyDMLib() {

    if (!(devLayer_.get()) || devLayer_.get() == nullptr) {
      DM_VLOG(HIGH) << "Device Layer pointer is null!" << std::endl;
      return -EAGAIN;
    }

    dmi = getInstance();

    if (!dmi) {
      DM_VLOG(HIGH) << "Device Management instance is null!" << std::endl;
      return -EAGAIN;
    }

    return 0;
  }

  void* handle_;
  std::unique_ptr<IDeviceLayer> devLayer_;
  getDM_t dmi;
};

static std::string cmd;
static bool cmd_flag = false;

static uint32_t code;
static bool code_flag = false;

static uint32_t node = 0;
static bool node_flag = true;

static uint32_t timeout = 70000;
static bool timeout_flag = true;

static power_state_e power_state;
static bool active_power_management_flag = false;

static uint8_t tdp_level;
static bool tdp_level_flag = false;

static pcie_reset_e pcie_reset;
static bool pcie_reset_flag = false;

static pcie_link_speed_e pcie_link_speed;
static bool pcie_link_speed_flag = false;

static pcie_lane_w_split_e pcie_lane_width;
static bool pcie_lane_width_flag = false;

static uint8_t sw_temperature_c;
static bool thresholds_flag = false;

static uint8_t mem_count;
static bool mem_count_flag = false;

static std::string imagePath;

static uint16_t minion_freq_mhz;
static uint16_t noc_freq_mhz;
static bool frequencies_flag = false;

static uint8_t volt_enc;
static device_mgmt_api::module_e volt_type;
static bool voltage_flag = false;

static uint32_t partid;
static uint32_t partid_flag = false;

// A namespace containing template for `bit_cast`. To be removed when `bit_cast` will be available
namespace templ {
template <class To, class From>
typename std::enable_if_t<
  sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, To>
// constexpr support needs compiler magic
bit_cast(const From& src) noexcept {
  static_assert(std::is_trivially_constructible_v<To>,
                "This implementation additionally requires destination type to be trivially constructible");

  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
} // namespace templ

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
bool decodeTraceEvents(int deviceIdx, const std::vector<std::byte>& traceBuf, TraceBufferType bufferType) {
  if (traceBuf.empty()) {
    DM_LOG(INFO) << "Invalid trace buffer! size is 0";
    return false;
  }
  std::ofstream logfile;
  std::string fileName = (fs::path("dev" + std::to_string(deviceIdx))).string();
  fileName += "traces.txt";
  DM_LOG(INFO) << "Saving trace to file: " << fileName;
  logfile.open(fileName, std::ios_base::app);
  switch (bufferType) {
  case TraceBufferType::TraceBufferSP:
    logfile << "-> SP Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferMM:
    logfile << "-> MM S-Mode Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferCM:
    logfile << "-> CM S-Mode Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferSPStats:
    logfile << "-> SP Stats Traces" << std::endl;
    break;
  case TraceBufferType::TraceBufferMMStats:
    logfile << "-> MM Stats Traces" << std::endl;
    break;
  default:
    DM_LOG(INFO) << "Cannot decode unknown buffer type!";
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

void dumpRawTraceBuffer(int deviceIdx, const std::vector<std::byte>& traceBuf, TraceBufferType bufferType) {
  if (traceBuf.empty()) {
    DM_LOG(INFO) << "Invalid trace buffer! size is 0";
    return;
  }
  struct trace_buffer_std_header_t* traceHdr;
  std::string fileName = (fs::path("dev" + std::to_string(deviceIdx) + "_")).string();
  unsigned int dataSize = 0;
  auto fileFlags = std::ofstream::binary;
  // Select the bin file
  switch (bufferType) {
  case TraceBufferType::TraceBufferSP:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "sp_traces";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferSPStats:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    DM_LOG(INFO) << "data size " << dataSize;
    fileName += "sp_stats";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferMM:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "mm_traces";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferMMStats:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->data_size;
    fileName += "mm_stats";
    fileFlags |= std::ios_base::app;
    break;
  case TraceBufferType::TraceBufferCM:
    traceHdr = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf.data());
    dataSize = traceHdr->sub_buffer_count * traceHdr->sub_buffer_size;
    fileName += "cmsmode_traces";
    fileFlags |= std::ofstream::trunc;
    break;
  default:
    DM_LOG(INFO) << "Cannot dump unknown buffer type!";
    return;
  }

  fileName += ".bin";

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
    DM_LOG(INFO) << "Unable to open file: " << fileName;
  }
}

int runService(const char* input_buff, const uint32_t input_size, char* output_buff, const uint32_t output_size) {

  static DMLib dml;
  int ret;

  ret = dml.verifyDMLib();
  if (ret != DM_STATUS_SUCCESS) {
    DM_VLOG(HIGH) << "Failed to verify the DM lib: " << ret << std::endl;
    return ret;
  }
  DeviceManagement& dm = (*dml.dmi)(dml.devLayer_.get());

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ret = dm.serviceRequest(node, code, input_buff, input_size, output_buff, output_size, hst_latency.get(),
                          dev_latency.get(), timeout);

  if (ret != DM_STATUS_SUCCESS) {
    DM_LOG(INFO) << "Service request failed with return code: " << ret << std::endl;
    return ret;
  }

  DM_LOG(INFO) << "Host Latency: " << *hst_latency << " ms" << std::endl;
  DM_LOG(INFO) << "Device Latency: " << *dev_latency << " us" << std::endl;
  DM_LOG(INFO) << "Service request succeeded" << std::endl;
  return 0;
}

int verifyService() {
  int ret;

  switch (code) {
  case DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME:
  case DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER:
  case DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER:
  case DM_CMD::DM_CMD_GET_MODULE_REVISION:
  case DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR:
  case DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
  case DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE: {
    const uint32_t output_size = sizeof(struct asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
    output_buff[output_size - 1] = '\0';
    std::string str_output = std::string(output_buff);

    DM_LOG(INFO) << "Asset Output: " << str_output << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_PART_NUMBER: {
    if (!partid_flag) {
      DM_VLOG(HIGH) << "Aborting, --partid was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(uint32_t);
    char input_buff[input_size] = {0};
    *((uint32_t*)input_buff) = partid;

    if ((ret = runService(input_buff, input_size, nullptr, 0)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
    const uint32_t output_size = sizeof(struct asset_info_t);
    char output_buff[output_size] = {0};
    char pcie_speed[32];

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    try {
      switch (std::stoi(output_buff, nullptr, 10)) {
      case 2:
        strncpy(pcie_speed, "PCIE_GEN1", sizeof(pcie_speed));
        break;
      case 5:
        strncpy(pcie_speed, "PCIE_GEN2", sizeof(pcie_speed));
        break;
      case 8:
        strncpy(pcie_speed, "PCIE_GEN3", sizeof(pcie_speed));
        break;
      case 16:
        strncpy(pcie_speed, "PCIE_GEN4", sizeof(pcie_speed));
        break;
      case 32:
        strncpy(pcie_speed, "PCIE_GEN5", sizeof(pcie_speed));
        break;
      }
    } catch (const std::invalid_argument& ia) {
      DM_LOG(INFO) << ia.what() << "Invalid resposne from the device= " << output_buff << std::endl;
      return -EINVAL;
    }

    DM_LOG(INFO) << "PCIE Speed: " << pcie_speed << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB: {
    const uint32_t output_size = sizeof(struct asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "DDR Memory Size: " << (int)(*output_buff) << "GB" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION: {
    const uint32_t output_size = sizeof(struct asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    try {
      std::string str_output = std::string(output_buff, output_size);
      DM_LOG(INFO) << "ASIC Revision: " << std::stoi(str_output, nullptr, 16) << std::endl;
    } catch (const std::invalid_argument& ia) {
      DM_LOG(INFO) << "Invalid response from device: " << ia.what() << '\n';
      return -EINVAL;
    }

  } break;

  case DM_CMD::DM_CMD_GET_MODULE_POWER_STATE: {
    const uint32_t output_size = sizeof(power_state_e);
    char output_buff[output_size] = {0};
    char power_state[32];

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    switch (POWER_STATE(*output_buff)) {
    case 0:
      strncpy(power_state, "POWER_STATE_MAX_POWER", sizeof(power_state));
      break;
    case 1:
      strncpy(power_state, "POWER_STATE_MANAGED_POWER", sizeof(power_state));
      break;
    case 2:
      strncpy(power_state, "POWER_STATE_SAFE_POWER", sizeof(power_state));
      break;
    case 3:
      strncpy(power_state, "POWER_STATE_LOW_POWER", sizeof(power_state));
      break;
    default:
      DM_LOG(INFO) << "Invalid power state: " << std::endl;
      break;
    }

    DM_LOG(INFO) << "Power State Output: " << power_state << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT: {
    if (!active_power_management_flag) {
      DM_VLOG(HIGH) << "Aborting, --active_pwr_mgmt was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(power_state_e);
    const char input_buff[input_size] = {
      (char)active_power_management_flag}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};
    uint8_t tdp_level = 0;

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    tdp_level = (uint8_t)output_buff[0];
    DM_LOG(INFO) << "TDP Level Output: " << +tdp_level << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL: {
    if (!tdp_level_flag) {
      DM_VLOG(HIGH) << "Aborting, --tdplevel was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {(char)tdp_level}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS: {
    const uint32_t output_size = sizeof(temperature_threshold_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    temperature_threshold_t* temperature_threshold = (temperature_threshold_t*)output_buff;
    DM_LOG(INFO) << "SW Managed Temperature Threshold Output: " << +temperature_threshold->sw_temperature_c << " c"
                 << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS: {
    if (!thresholds_flag) {
      DM_VLOG(HIGH) << "Aborting, --thresholds was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(temperature_threshold_t);
    const char input_buff[input_size] = {(char)sw_temperature_c}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE: {
    const uint32_t output_size = sizeof(current_temperature_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    current_temperature_t* cur_temp = (current_temperature_t*)output_buff;
    DM_LOG(INFO) << "PMIC SYS Temperature Output: " << +cur_temp->pmic_sys << " c" << std::endl;
    DM_LOG(INFO) << "IOSHIRE Current Temperature Output: " << +cur_temp->ioshire_current << " c" << std::endl;
    DM_LOG(INFO) << "IOSHIRE Low Temperature Output: " << +cur_temp->ioshire_low << " c" << std::endl;
    DM_LOG(INFO) << "IOSHIRE High Temperature Output: " << +cur_temp->ioshire_high << " c" << std::endl;
    DM_LOG(INFO) << "MINSHIRE Current Temperature Output: " << +cur_temp->minshire_avg << " c" << std::endl;
    DM_LOG(INFO) << "MINSHIRE Low Temperature Output: " << +cur_temp->minshire_low << " c" << std::endl;
    DM_LOG(INFO) << "MINSHIRE High Temperature Output: " << +cur_temp->minshire_high << " c" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES: {
    std::string throttle_state_name[7] = {"POWER_THROTTLE_STATE_POWER_IDLE",   "POWER_THROTTLE_STATE_THERMAL_IDLE",
                                          "POWER_THROTTLE_STATE_POWER_UP",     "POWER_THROTTLE_STATE_POWER_DOWN",
                                          "POWER_THROTTLE_STATE_THERMAL_DOWN", "POWER_THROTTLE_STATE_POWER_SAFE",
                                          "POWER_THROTTLE_STATE_THERMAL_SAFE"};
    for (device_mgmt_api::power_throttle_state_e throttle_state = device_mgmt_api::POWER_THROTTLE_STATE_POWER_UP;
         throttle_state <= device_mgmt_api::POWER_THROTTLE_STATE_THERMAL_SAFE; throttle_state++) {
      const uint32_t input_size = sizeof(device_mgmt_api::power_throttle_state_e);
      const char input_buff[input_size] = {(char)throttle_state};

      const uint32_t output_size = sizeof(residency_t);
      char output_buff[output_size] = {0};

      if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
        return ret;
      }

      residency_t* residency = (residency_t*)output_buff;
      DM_LOG(INFO) << "throttle_residency " << throttle_state_name[throttle_state] << "(in usecs):" << std::endl;
      DM_LOG(INFO) << "cumulative: " << residency->cumulative << std::endl;
      DM_LOG(INFO) << "average: " << residency->average << std::endl;
      DM_LOG(INFO) << "maximum: " << residency->maximum << std::endl;
      DM_LOG(INFO) << "minimum: " << residency->minimum << std::endl;
    }
  } break;

  case DM_CMD::DM_CMD_SET_FREQUENCY: {
    if (!frequencies_flag) {
      DM_VLOG(HIGH) << "Aborting, --frequencies was not defined" << std::endl;
      return -EINVAL;
    }
    for (device_mgmt_api::pll_id_e pll_id = device_mgmt_api::PLL_ID_NOC_PLL;
         pll_id <= device_mgmt_api::PLL_ID_MINION_PLL; pll_id++) {
      uint16_t pll_freq;
      if (pll_id == device_mgmt_api::PLL_ID_NOC_PLL) {
        pll_freq = noc_freq_mhz;
      } else if (pll_id == device_mgmt_api::PLL_ID_MINION_PLL) {
        pll_freq = minion_freq_mhz;
      }
      const uint32_t input_size =
        sizeof(device_mgmt_api::pll_id_e) + sizeof(uint16_t) + sizeof(device_mgmt_api::use_step_e);
      char input_buff[input_size];
      input_buff[0] = (char)(pll_freq & 0xff);
      input_buff[1] = (char)((pll_freq >> 8) & 0xff);
      input_buff[2] = (char)pll_id;
      input_buff[3] = (char)device_mgmt_api::USE_STEP_CLOCK_TRUE;

      if ((ret = runService(input_buff, input_size, nullptr, 0)) != DM_STATUS_SUCCESS) {
        return ret;
      }
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_POWER: {
    const uint32_t output_size = sizeof(module_power_t);
    char output_buff[output_size] = {0};
    float power;

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_power_t* module_power = (module_power_t*)output_buff;
    power = ((float)module_power->power / 1000);
    DM_LOG(INFO) << "Module Power Output: " << power << " W" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_VOLTAGE: {
    const uint32_t output_size = sizeof(module_voltage_t);
    char output_buff[output_size] = {0};
    uint32_t voltage;

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_voltage_t* module_voltage = (module_voltage_t*)output_buff;

    voltage = BIN2VOLTAGE(module_voltage->ddr, 250, 5);
    DM_LOG(INFO) << "Module Voltage DDR: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->l2_cache, 250, 5);
    DM_LOG(INFO) << "Module Voltage L2CACHE: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->maxion, 250, 5);
    DM_LOG(INFO) << "Module Voltage MAXION: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->minion, 250, 5);
    DM_LOG(INFO) << "Module Voltage MINION: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->pcie, 600, 6); // FIXME its 6.25 actualy, try float
    DM_LOG(INFO) << "Module Voltage PCIE: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->noc, 250, 5);
    DM_LOG(INFO) << "Module Voltage NOC: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->pcie_logic, 600, 6);

  } break;

  case DM_CMD::DM_CMD_SET_MODULE_VOLTAGE: {
    if (!voltage_flag) {
      DM_VLOG(HIGH) << "Aborting, --voltage was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(volt_type) + sizeof(volt_enc);
    char input_buff[input_size] = {0};
    input_buff[0] = (char)device_mgmt_api::MODULE_DDR;
    input_buff[1] = (char)volt_enc;

    if ((ret = runService(input_buff, input_size, nullptr, 0)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_UPTIME: {
    const uint32_t output_size = sizeof(module_uptime_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_uptime_t* module_uptime = (module_uptime_t*)output_buff;
    DM_LOG(INFO) << "Module Uptime Output: " << module_uptime->day << " d " << +module_uptime->hours << " h "
                 << +module_uptime->mins << " m" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE: {
    const uint32_t output_size = sizeof(max_temperature_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_temperature_t* max_temperature = (max_temperature_t*)output_buff;
    DM_LOG(INFO) << "Module Max Temperature Output: " << +max_temperature->max_temperature_c << " c" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR: {
    const uint32_t output_size = sizeof(max_ecc_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_ecc_count_t* max_ecc_count = (max_ecc_count_t*)output_buff;
    DM_LOG(INFO) << "Max Memory Error Output: " << +max_ecc_count->count << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW: {
    const uint32_t output_size = sizeof(max_dram_bw_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_dram_bw_t* max_dram_bw = (max_dram_bw_t*)output_buff;
    DM_LOG(INFO) << "Module Max DDR BW Read Output: " << +max_dram_bw->max_bw_rd_req_sec << " GB/s" << std::endl;
    DM_LOG(INFO) << "Module Max DDR BW Write Output: " << +max_dram_bw->max_bw_wr_req_sec << " GB/s" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES: {
    std::string power_state_name[4] = {"POWER_STATE_MAX_POWER", "POWER_STATE_MANAGED_POWER", "POWER_STATE_SAFE_POWER",
                                       "POWER_STATE_LOW_POWER"};
    for (device_mgmt_api::power_state_e power_state = device_mgmt_api::POWER_STATE_MAX_POWER;
         power_state <= device_mgmt_api::POWER_STATE_SAFE_POWER; power_state++) {
      const uint32_t input_size = sizeof(device_mgmt_api::power_state_e);
      const char input_buff[input_size] = {(char)power_state};

      const uint32_t output_size = sizeof(residency_t);
      char output_buff[output_size] = {0};

      if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
        return ret;
      }

      residency_t* residency = (residency_t*)output_buff;
      DM_LOG(INFO) << "power_residency " << power_state_name[power_state] << "(in usecs):" << std::endl;
      DM_LOG(INFO) << "cumulative: " << residency->cumulative << std::endl;
      DM_LOG(INFO) << "average: " << residency->average << std::endl;
      DM_LOG(INFO) << "maximum: " << residency->maximum << std::endl;
      DM_LOG(INFO) << "minimum: " << residency->minimum << std::endl;
    }
  } break;

  case DM_CMD::DM_CMD_SET_DDR_ECC_COUNT:
  case DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT:
  case DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT: {
    if (!mem_count_flag) {
      DM_VLOG(HIGH) << "Aborting, --memcount was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {(char)mem_count}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_PCIE_RESET: {
    if (!pcie_reset_flag) {
      DM_VLOG(HIGH) << "Aborting, --pciereset was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(pcie_reset_e);
    const char input_buff[input_size] = {(char)pcie_reset}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC: {
    const uint32_t output_size = sizeof(errors_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    errors_count_t* errors_count = (errors_count_t*)output_buff;
    DM_LOG(INFO) << "Module PCIE ECC Output: " << +errors_count->ecc << std::endl;
    DM_LOG(INFO) << "Module PCIE UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER: {
    const uint32_t output_size = sizeof(dram_bw_counter_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    dram_bw_counter_t* dram_bw_counter = (dram_bw_counter_t*)output_buff;
    DM_LOG(INFO) << "Module DDR BW Read Counter Output: " << dram_bw_counter->bw_rd_req_sec << std::endl;
    DM_LOG(INFO) << "Module DDR BW Write Counter Output: " << dram_bw_counter->bw_wr_req_sec << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC: {
    const uint32_t output_size = sizeof(errors_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    errors_count_t* errors_count = (errors_count_t*)output_buff;
    DM_LOG(INFO) << "Module DDR ECC Output: " << +errors_count->ecc << std::endl;
    DM_LOG(INFO) << "Module DDR UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC: {
    const uint32_t output_size = sizeof(errors_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    errors_count_t* errors_count = (errors_count_t*)output_buff;
    DM_LOG(INFO) << "Module SRAM ECC Output: " << +errors_count->ecc << std::endl;
    DM_LOG(INFO) << "Module SRAM UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED: {
    if (!pcie_link_speed_flag) {
      DM_VLOG(HIGH) << "Aborting, --pciespeed was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(pcie_link_speed_e);
    const char input_buff[input_size] = {(char)pcie_link_speed}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH: {
    if (!pcie_lane_width_flag) {
      DM_VLOG(HIGH) << "Aborting, --pciewidth was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(pcie_lane_w_split_e);
    const char input_buff[input_size] = {(char)pcie_lane_width}; // bounds check prevents issues with narrowing

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_PCIE_RETRAIN_PHY: {
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = {0};

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES: {
    const uint32_t output_size = sizeof(asic_frequencies_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    asic_frequencies_t* asic_frequencies = (asic_frequencies_t*)output_buff;
    DM_LOG(INFO) << "ASIC Frequency Minion Shire: " << asic_frequencies->minion_shire_mhz << " Mhz" << std::endl;
    DM_LOG(INFO) << "ASIC Frequency NOC: " << asic_frequencies->noc_mhz << " Mhz" << std::endl;
    DM_LOG(INFO) << "ASIC Frequency DDR: " << asic_frequencies->ddr_mhz << " Mhz" << std::endl;
    DM_LOG(INFO) << "ASIC Frequency PCIE Shire: " << asic_frequencies->pcie_shire_mhz << " Mhz" << std::endl;
    DM_LOG(INFO) << "ASIC Frequency IO Shire: " << asic_frequencies->io_shire_mhz << " Mhz" << std::endl;
    DM_LOG(INFO) << "ASIC Frequency Mem Shire: " << asic_frequencies->mem_shire_mhz << " Mhz" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH: {
    const uint32_t output_size = sizeof(dram_bw_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    dram_bw_t* dram_bw = (dram_bw_t*)output_buff;
    DM_LOG(INFO) << "DRAM Bandwidth Read Output: " << dram_bw->read_req_sec << " GB/s" << std::endl;
    DM_LOG(INFO) << "DRAM Bandwidth Write Output: " << dram_bw->write_req_sec << " GB/s" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION: {
    const uint32_t output_size = sizeof(percentage_cap_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    percentage_cap_t* percentage_cap = (percentage_cap_t*)output_buff;
    DM_LOG(INFO) << "DRAM Capacity Utilization Output: " << percentage_cap->pct_cap << " %" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "ASIC per Core Datapath Utilization Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_UTILIZATION: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "ASIC Utilization Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_STALLS: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "ASIC Stalls Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_LATENCY: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "ASIC Latency Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MM_ERROR_COUNT: {
    const uint32_t output_size = sizeof(mm_error_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    mm_error_count_t* mm_error_count = (mm_error_count_t*)output_buff;
    DM_LOG(INFO) << "MM Hang Count: " << +mm_error_count->hang_count << std::endl;
    DM_LOG(INFO) << "MM Exception Count: " << +mm_error_count->exception_count << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS: {
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DM_LOG(INFO) << "Firmware Boot Status: Success! " << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS: {
    const uint32_t output_size = sizeof(device_mgmt_api::firmware_version_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    device_mgmt_api::firmware_version_t* firmware_versions = (device_mgmt_api::firmware_version_t*)output_buff;

    uint32_t versions = firmware_versions->fw_release_rev;
    DM_LOG(INFO) << "Firmware release revision: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->bl1_v;
    DM_LOG(INFO) << "BL1 Firmware versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->bl2_v;
    DM_LOG(INFO) << "BL2 Firmware versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->pmic_v;
    DM_LOG(INFO) << "PMIC Firmware versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->mm_v;
    DM_LOG(INFO) << "Master Minion Firmware versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->wm_v;
    DM_LOG(INFO) << "Worker Minion versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

    versions = firmware_versions->machm_v;
    DM_LOG(INFO) << "Machine Minion versions: Major: " << ((versions >> 24) & 0xFF)
                 << " Minor: " << ((versions >> 16) & 0xFF) << " Revision: " << ((versions >> 8) & 0xFF) << std::endl;

  } break;

  case DM_CMD::DM_CMD_SET_FIRMWARE_VERSION_COUNTER: {
    const uint32_t input_size = sizeof(uint256_t);
    const char input_buff[input_size] = {123}; // TODO: provide the counter by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT: {
    const uint32_t input_size = sizeof(uint512_t);
    const char input_buff[input_size] = {123}; // TODO: provide the hash by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_SW_BOOT_ROOT_CERT: {
    const uint32_t input_size = sizeof(uint512_t);
    const char input_buff[input_size] = {123}; // TODO: provide the hash by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_GET_FUSED_PUBLIC_KEYS: {
    const uint32_t output_size = sizeof(device_mgmt_api::fused_public_keys_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    device_mgmt_api::fused_public_keys_t* fused_public_key = (device_mgmt_api::fused_public_keys_t*)output_buff;
    try {
      DM_LOG(INFO) << "Public keys: " << std::endl;
      for (unsigned int i = 0; i < output_size; ++i)
        DM_LOG(INFO) << fused_public_key->keys[i] << " ";
      DM_LOG(INFO) << std::endl;
    } catch (const std::invalid_argument& ia) {
      DM_LOG(INFO) << "SP ROOT HASH wasn't written to OTP " << '\n';
    }
  } break;

  case DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE: {
    // Unique case in which two service requests are done, first one to reset SP
    // trace buffer followed by second one to perform firmware update. For this
    // reason runService() isn't used.

    DMLib dml;

    ret = dml.verifyDMLib();
    if (ret != DM_STATUS_SUCCESS) {
      DM_VLOG(HIGH) << "Failed to verify the DM lib: " << ret << std::endl;
      return ret;
    }
    DeviceManagement& dm = (*dml.dmi)(dml.devLayer_.get());

    std::array<char, sizeof(device_mgmt_api::trace_control_e)> input_buff;
    device_mgmt_api::trace_control_e control =
      device_mgmt_api::TRACE_CONTROL_TRACE_ENABLE | device_mgmt_api::TRACE_CONTROL_RESET_TRACEBUF;
    memcpy(input_buff.data(), &control, sizeof(control));

    auto hst_latency = std::make_unique<uint32_t>();
    auto dev_latency = std::make_unique<uint64_t>();

    ret = dm.serviceRequest(node, DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL, input_buff.data(), input_buff.size(),
                            nullptr, 0, hst_latency.get(), dev_latency.get(), timeout);

    if (ret != DM_STATUS_SUCCESS) {
      DM_LOG(INFO) << "Service request failed with return code: " << ret << std::endl;
      return ret;
    }

    DM_LOG(INFO) << "Host Latency: " << *hst_latency << " ms" << std::endl;
    DM_LOG(INFO) << "Device Latency: " << *dev_latency << " us" << std::endl;
    DM_LOG(INFO) << "Service request succeeded" << std::endl;

    ret = dm.serviceRequest(node, code, imagePath.c_str(), imagePath.length(), nullptr, 0, hst_latency.get(),
                            dev_latency.get(), timeout);

    if (ret != DM_STATUS_SUCCESS) {
      DM_LOG(INFO) << "Service request failed with return code: " << ret << std::endl;
      return ret;
    }

    DM_LOG(INFO) << "Host Latency: " << *hst_latency << " ms" << std::endl;
    DM_LOG(INFO) << "Device Latency: " << *dev_latency << " us" << std::endl;
    DM_LOG(INFO) << "Service request succeeded" << std::endl;
    ret = 0;
  } break;

  case DM_CMD::DM_CMD_MM_RESET: {
    if ((ret = runService(nullptr, 0, nullptr, 0)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  default:
    DM_VLOG(HIGH) << "Aborting, command: " << cmd << " (" << code << ") is currently unsupported" << std::endl;
    return -EINVAL;
    break;
  }

  return ret;
}

bool validDigitsOnly() {
  std::string str_optarg = std::string(optarg);
  std::regex re("^[0-9]+$");
  std::smatch m;
  if (!std::regex_search(str_optarg, m, re)) {
    DM_VLOG(HIGH) << "Aborting, argument: " << str_optarg << " is not valid. It contains more than just digits"
                  << std::endl;
    return false;
  }

  return true;
}

bool validCommand() {
  std::string str_optarg = std::string(optarg);
  std::regex re("^[a-zA-Z_]+$");
  std::smatch m;
  if (!std::regex_search(str_optarg, m, re)) {
    DM_VLOG(HIGH) << "Aborting, command: " << str_optarg << " is not valid ( ^[a-zA-Z_]+$ )" << std::endl;
    return false;
  }

  auto it = commandCodeTable.find(str_optarg);

  if (it != commandCodeTable.end()) {
    cmd = it->first;
    code = it->second;
    DM_VLOG(HIGH) << "command: " << str_optarg << " code " << code << std::endl;
    return true;
  }

  DM_VLOG(HIGH) << "Aborting, command: " << str_optarg << " not found" << std::endl;
  return false;
}

bool validCode() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto tmp_optarg = std::strtoul(optarg, &end, 10);

  if (end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, command: " << optarg << " is not valid" << std::endl;
    return false;
  }

  for (auto it = commandCodeTable.begin(); it != commandCodeTable.end(); ++it) {
    if (tmp_optarg == it->second) {
      cmd = it->first;
      code = it->second;
      return true;
    }
  }

  DM_VLOG(HIGH) << "Aborting, command: " << optarg << " not found" << std::endl;
  return false;
}

bool validMemCount() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto count = std::strtoul(optarg, &end, 10);

  if (count > SCHAR_MAX || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not valid ( 0-" << SCHAR_MAX << " )" << std::endl;
    return false;
  }

  mem_count = (uint8_t)count;

  return true;
}

bool validNode() {
  std::string str_optarg = std::string(optarg);
  std::regex re("^[0-5]{1}$");
  std::smatch m;
  if (!std::regex_search(str_optarg, m, re)) {
    DM_VLOG(HIGH) << "Aborting, node: " << str_optarg << " is not valid ( ^[0-5]{1}$ )" << std::endl;
    return false;
  }

  char* end;
  errno = 0;

  node = std::strtoul(optarg, &end, 10);

  if (node > 5 || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not a valid device node ( 0-5 )" << std::endl;
    return false;
  }

  return true;
}

bool validActivePowerManagement() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto state = std::strtoul(optarg, &end, 10);

  if (state > 1 || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not a valid active power management ( 0-1 )" << std::endl;
    return false;
  }

  power_state = (power_state_e)state;

  return true;
}

bool validLaneWidth() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  pcie_lane_width = std::strtoul(optarg, &end, 10);

  if (pcie_lane_width > 1 || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not a valid pcie lane width ( 0-1 )" << std::endl;
    return false;
  }

  return true;
}

bool validLinkSpeed() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  pcie_link_speed = std::strtoul(optarg, &end, 10);

  if (pcie_link_speed > 1 || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not a valid pcie link speed ( 0-1 )" << std::endl;
    return false;
  }

  return true;
}

bool validReset() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  pcie_reset = std::strtoul(optarg, &end, 10);

  if (pcie_reset > 2 || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not a valid pcie reset type ( 0-2 )" << std::endl;
    return false;
  }

  return true;
}

#define TDP_LEVEL_MAX 40

bool validTDPLevel() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto level = std::strtoul(optarg, &end, 10);

  if (level > TDP_LEVEL_MAX || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not valid tdp level ( 0-40 )" << std::endl;
    return false;
  }

  tdp_level = level;

  return true;
}

bool validThresholds() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto lo = std::strtoul(optarg, &end, 10);

  if (lo > SCHAR_MAX || end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << lo << " is not valid ( 0-" << SCHAR_MAX << " )" << std::endl;
    return false;
  }

  errno = 0;

  sw_temperature_c = (uint8_t)lo;

  return true;
}

bool validTimeout() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  timeout = std::strtoul(optarg, &end, 10);

  if (end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not valid ( 0 - " << ULONG_MAX << " )" << std::endl;
    return false;
  }

  return true;
}

bool validPath() {
  if (!std::experimental::filesystem::exists(std::string(optarg))) {
    DM_VLOG(HIGH) << "The file doesn't exist" << std::string(optarg) << std::endl;
    return false;
  }
  imagePath.assign(std::string(optarg));

  return true;
}

bool validFrequencies() {
  if (!std::regex_match(std::string(optarg), std::regex("^[0-9]+,[0-9]+$"))) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not valid, e.g: 300,100" << std::endl;
    return false;
  }

  auto freq = std::stoul(std::strtok(optarg, ","));
  if (freq > std::numeric_limits<decltype(minion_freq_mhz)>::max()) {
    DM_VLOG(HIGH) << "Aborting, minion shire frequency (MHz): " << freq << " is not valid ( 0 - "
                  << std::numeric_limits<decltype(minion_freq_mhz)>::max() << " )" << std::endl;
    return false;
  }
  minion_freq_mhz = static_cast<decltype(minion_freq_mhz)>(freq);

  freq = std::stoul(std::strtok(NULL, ","));
  if (freq > std::numeric_limits<decltype(noc_freq_mhz)>::max()) {
    DM_VLOG(HIGH) << "Aborting, noc frequency (MHz): " << freq << " is not valid ( 0 - "
                  << std::numeric_limits<decltype(noc_freq_mhz)>::max() << " )" << std::endl;
    return false;
  }
  noc_freq_mhz = static_cast<decltype(noc_freq_mhz)>(freq);

  return true;
}

bool validVoltage() {
  if (!std::regex_match(std::string(optarg), std::regex("^[A-Z]+,[0-9]+$"))) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg
                  << " is not valid, arguments should be in this format: <MODULE_TYPE>,<VOLTAGE_VALUE_IN_MV>"
                  << std::endl;
    return false;
  }

  static const std::map<std::string,
                        std::tuple<device_mgmt_api::module_e, uint8_t /* base */, uint8_t /* multiplier */>>
    moduleTypes{
      {"DDR", {device_mgmt_api::MODULE_DDR, 250, 5}},
      {"L2CACHE", {device_mgmt_api::MODULE_L2CACHE, 250, 5}},
      {"MAXION", {device_mgmt_api::MODULE_MAXION, 250, 5}},
      {"MINION", {device_mgmt_api::MODULE_MINION, 250, 5}},
      {"PCIE", {device_mgmt_api::MODULE_PCIE, 600, 6}},
      {"NOC", {device_mgmt_api::MODULE_NOC, 250, 5}},
      {"PCIE_LOGIC", {device_mgmt_api::MODULE_PCIE_LOGIC, 600, 6}},
    };
  auto moduleType = std::strtok(optarg, ",");

  auto itr = moduleTypes.find(moduleType);
  if (itr == moduleTypes.end()) {
    DM_VLOG(HIGH) << "Aborting, Invalid Module Type: " << moduleType
                  << ", possible types are {DDR, L2CACHE, MAXION, MINION, PCIE, NOC, PCIE_LOGIC}" << std::endl;
    return false;
  }

  uint8_t base;
  uint8_t multiplier;
  std::tie(volt_type, base, multiplier) = itr->second;

  auto voltage = std::stoul(std::strtok(NULL, ","));
  if (voltage > std::numeric_limits<uint16_t>::max()) {
    DM_VLOG(HIGH) << "Aborting, Voltage : " << voltage << " is not valid ( 0 - " << std::numeric_limits<uint16_t>::max()
                  << " )" << std::endl;
    return false;
  }
  volt_enc = static_cast<decltype(volt_enc)>(VOLTAGE2BIN(voltage, base, multiplier));

  return true;
}

bool validPartId() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  partid = std::strtoul(optarg, &end, 10);

  if (end == optarg || *end != '\0' || errno != 0) {
    DM_VLOG(HIGH) << "Aborting, argument: " << optarg << " is not valid ( 0 - "
                  << std::numeric_limits<decltype(partid)>::max() << " )" << std::endl;
    return false;
  }

  return true;
}

static struct option long_options[] = {{"code", required_argument, 0, 'o'},
                                       {"command", required_argument, 0, 'm'},
                                       {"help", no_argument, 0, 'h'},
                                       {"memcount", required_argument, 0, 'c'},
                                       {"node", required_argument, 0, 'n'},
                                       {"active_pwr_mgmt", required_argument, 0, 'p'},
                                       {"pciereset", required_argument, 0, 'r'},
                                       {"pciespeed", required_argument, 0, 's'},
                                       {"pciewidth", required_argument, 0, 'w'},
                                       {"tdplevel", required_argument, 0, 't'},
                                       {"thresholds", required_argument, 0, 'e'},
                                       {"timeout", required_argument, 0, 'u'},
                                       {"imagepath", required_argument, 0, 'i'},
                                       {"gettrace", required_argument, 0, 'g'},
                                       {"frequencies", required_argument, 0, 'f'},
                                       {"version", required_argument, 0, 'V'},
                                       {"voltage", required_argument, 0, 'v'},
                                       {"partid", required_argument, 0, 'd'},
                                       {0, 0, 0, 0}};

void printCode(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[0].val << ", --" << long_options[0].name << "=ncode" << std::endl;
  std::cout << "\t\t"
            << "Command by ID (see below)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME
            << std::endl;
}

void printCommand(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[1].val << ", --" << long_options[1].name << "=command" << std::endl;
  std::cout << "\t\t"
            << "Command by name:" << std::endl;
  std::cout << std::endl;

  // Declare vector of pairs
  std::vector<std::pair<std::string, device_mgmt_api::DM_CMD>> A;

  // Copy key-value pair from Map to vector of pairs
  for (auto& it : commandCodeTable) {
    A.push_back(it);
  }
  std::sort(A.begin(), A.end(),
            [](std::pair<std::string, device_mgmt_api::DM_CMD>& a, std::pair<std::string, device_mgmt_api::DM_CMD>& b) {
              return a.first < b.first;
            });
  for (auto const& [key, val] : A) {
    if (val >= DM_CMD_SET_DM_TRACE_RUN_CONTROL && val <= DM_CMD_SET_DM_TRACE_CONFIG) {
      continue;
    }
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_GET_MODULE_MANUFACTURE_NAME"
            << std::endl;
}

void printNode(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[4].val << ", --" << long_options[4].name << "=node" << std::endl;
  std::cout << "\t\t"
            << "Device node by index" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME
            << " -" << (char)long_options[4].val << " 0" << std::endl;
}

void printTimeout(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[11].val << ", --" << long_options[11].name << "=nmsecs" << std::endl;
  std::cout << "\t\t"
            << "timeout in miliseconds" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME
            << " -" << (char)long_options[11].val << " 70000" << std::endl;
}

void printHelp(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[2].val << ", --" << long_options[2].name << std::endl;
  std::cout << "\t\t"
            << "Print usage; this output" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[2].val << std::endl;
}

void printMemCount(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[3].val << ", --" << long_options[3].name << "=ncount" << std::endl;
  std::cout << "\t\t"
            << "Set memory ECC count for DDR, SRAM, or PCIE (ex. 0)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_DDR_ECC_COUNT << " -"
            << (char)long_options[3].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_DDR_ECC_COUNT"
            << " -" << (char)long_options[3].val << " 0" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT << " -"
            << (char)long_options[3].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_PCIE_ECC_COUNT"
            << " -" << (char)long_options[3].val << " 0" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT << " -"
            << (char)long_options[3].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_SRAM_ECC_COUNT"
            << " -" << (char)long_options[3].val << " 0" << std::endl;
}

void printActivePowerManagement(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[5].val << ", --" << long_options[5].name << "=active_pwr_m" << std::endl;
  std::cout << "\t\t"
            << "Set active power management:" << std::endl;
  std::cout << std::endl;

  for (auto const& [key, val] : activePowerManagementTable) {
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " "
            << DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT << " -" << (char)long_options[5].val << " 0"
            << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " SET_MODULE_ACTIVE_POWER_MANAGEMENT"
            << " -" << (char)long_options[5].val << " 0" << std::endl;
}

void printPCIEReset(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[6].val << ", --" << long_options[6].name << "=nreset" << std::endl;
  std::cout << "\t\t"
            << "Set reset type:" << std::endl;
  std::cout << std::endl;

  for (auto const& [key, val] : pcieResetTable) {
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_PCIE_RESET << " -"
            << (char)long_options[6].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_PCIE_RESET"
            << " -" << (char)long_options[6].val << " 0" << std::endl;
}

void printPCIELinkSpeed(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[7].val << ", --" << long_options[7].name << "=nspeed" << std::endl;
  std::cout << "\t\t"
            << "Set PCIE link speed:" << std::endl;
  std::cout << std::endl;

  for (auto const& [key, val] : pcieLinkSpeedTable) {
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED
            << " -" << (char)long_options[7].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_PCIE_MAX_LINK_SPEED"
            << " -" << (char)long_options[7].val << " 0" << std::endl;
}

void printPCIELaneWidth(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[8].val << ", --" << long_options[8].name << "=nwidth" << std::endl;
  std::cout << "\t\t"
            << "Set PCIE lane width:" << std::endl;
  std::cout << std::endl;

  for (auto const& [key, val] : pcieLaneWidthTable) {
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH << " -"
            << (char)long_options[8].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_PCIE_LANE_WIDTH"
            << " -" << (char)long_options[8].val << " 0" << std::endl;
}

void printTDPLevel(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[9].val << ", --" << long_options[9].name << "=nlevel" << std::endl;
  std::cout << "\t\t"
            << "Set TDP level:" << std::endl;
  std::cout << std::endl;

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL
            << " -" << (char)long_options[9].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_STATIC_TDP_LEVEL"
            << " -" << (char)long_options[9].val << " 0" << std::endl;
}

void printThresholds(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[10].val << ", --" << long_options[10].name << "=nswtemp" << std::endl;
  std::cout << "\t\t"
            << "Set temperature thresholds (sw)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " "
            << DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS << " -" << (char)long_options[10].val << " 80"
            << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS"
            << " -" << (char)long_options[10].val << " 80" << std::endl;
}

void printFWUpdate(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[12].val << ", --" << long_options[12].name << "=npath" << std::endl;
  std::cout << "\t\t"
            << "Set Firmware Update" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE << " -"
            << (char)long_options[12].val << " path-to-flash-image" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_FIRMWARE_UPDATE"
            << " -" << (char)long_options[12].val << " path-to-flash-image" << std::endl;
}

void printTraceBuf(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[13].val << ", --" << long_options[13].name << "=[SP, SPST, MM, MMST, CM]"
            << std::endl;
  std::cout << "\t\t"
            << "Get Trace Buffer for SP, MM and CM. SPST is for SP Stats and MMST is for MM Stats buffer." << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[13].val << " "
            << "[SP, SPST, MM, MMST, CM]" << std::endl;
}

void printFrequencies(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[14].val << ", --" << long_options[14].name << "=minionfreq,nocfreq"
            << std::endl;
  std::cout << "\t\t"
            << "Set Frequency (MHz)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_FREQUENCY << " -"
            << (char)long_options[14].val << " 400,200" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_FREQUENCY"
            << " -" << (char)long_options[14].val << " 400,200" << std::endl;
}

void printVersionUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[15].val << ", --" << long_options[15].name << std::endl;
  std::cout << "\t\t"
            << "Print DM Application version" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[15].val << std::endl;
}

void printVoltageUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[16].val << ", --" << long_options[16].name << "=module,voltage" << std::endl;
  std::cout << "\t\t"
            << "Set Voltage (mV)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_MODULE_VOLTAGE << " -"
            << (char)long_options[16].val << " DDR,715" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_VOLTAGE"
            << " -" << (char)long_options[16].val << " DDR,715" << std::endl;
}

void printPartIdUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[17].val << ", --" << long_options[17].name << "=module,voltage" << std::endl;
  std::cout << "\t\t"
            << "Set Part ID" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_MODULE_PART_NUMBER
            << " -" << (char)long_options[17].val << " 320" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_PART_NUMBER"
            << " -" << (char)long_options[17].val << " 320" << std::endl;
}

void printUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "Usage: " << argv << " -o ncode | -m command [-n node] [-u nmsecs] [-h]"
            << "[-c ncount | -p npower | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nswtemp | -i npath | -f "
               "minionfreq,nocfreq | -g tracebuf | -v module,voltage | -d partid | -V ]"
            << std::endl;
  printCode(argv);
  printCommand(argv);
  printNode(argv);
  printTimeout(argv);
  printHelp(argv);
  printMemCount(argv);
  printActivePowerManagement(argv);
  printPCIEReset(argv);
  printPCIELinkSpeed(argv);
  printPCIELaneWidth(argv);
  printTDPLevel(argv);
  printThresholds(argv);
  printFWUpdate(argv);
  printTraceBuf(argv);
  printFrequencies(argv);
  printVersionUsage(argv);
  printVoltageUsage(argv);
  printPartIdUsage(argv);
}

bool getTraceBuffer() {
  TraceBufferType buf_type;
  std::string str_optarg = std::string(optarg);

  std::unordered_map<std::string, TraceBufferType> const traceBuffers = {{"SP", TraceBufferType::TraceBufferSP},
                                                                         {"MM", TraceBufferType::TraceBufferMM},
                                                                         {"CM", TraceBufferType::TraceBufferCM},
                                                                         {"SPST", TraceBufferType::TraceBufferSPStats},
                                                                         {"MMST", TraceBufferType::TraceBufferMMStats}};

  auto it = traceBuffers.find(str_optarg);

  if (it != traceBuffers.end()) {
    buf_type = it->second;

    static DMLib dml;
    if (dml.verifyDMLib() != DM_STATUS_SUCCESS) {
      DM_VLOG(HIGH) << "Failed to verify the DM lib: " << std::endl;
      return false;
    }
    DeviceManagement& dm = (*dml.dmi)(dml.devLayer_.get());
    std::vector<std::byte> response;

    if (dm.getTraceBufferServiceProcessor(node, buf_type, response) != device_mgmt_api::DM_STATUS_SUCCESS) {
      DM_LOG(INFO) << "Unable to get trace buffer for node: " << node << std::endl;
      return false;
    }
    dumpRawTraceBuffer(node, response, buf_type);
    decodeTraceEvents(node, response, buf_type);

    return true;
  }
  DM_VLOG(HIGH) << "Not a valid argument for trace buffer: " << str_optarg << std::endl;
  return false;
}

void printVersion(void) {
  std::cout << "dev_mngt_service version " << DM_APP_VERSION << std::endl;
}

int main(int argc, char** argv) {
  int c;
  int option_index = 0;

  // Initialize Google's logging library.
  logging::LoggerDefault loggerDefault_;

  while (1) {

    c = getopt_long(argc, argv, "d:o:m:hc:n:p:r:s:w:t:e:u:i:g:f:v:V", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {

    case 'd':
      if (!(partid_flag = validPartId())) {
        return -EINVAL;
      }
      break;

    case 'o':
      if (cmd_flag) {
        DM_LOG(INFO) << "Command already provided, ignoring code: " << optarg << std::endl;
        break;
      }
      if (!(code_flag = validCode())) {
        return -EINVAL;
      }
      break;

    case 'm':
      if (code_flag) {
        DM_LOG(INFO) << "Code already provided, ignoring command: " << optarg << std::endl;
        break;
      }
      if (!(cmd_flag = validCommand())) {
        return -EINVAL;
      }
      break;

    case 'h':
      printUsage(argv[0]);
      return 0;

    case 'c':
      if (!(mem_count_flag = validMemCount())) {
        return -EINVAL;
      }
      break;

    case 'n':
      if (!(node_flag = validNode())) {
        return -EINVAL;
      }
      break;

    case 'p':
      if (!(active_power_management_flag = validActivePowerManagement())) {
        return -EINVAL;
      }
      break;

    case 'r':
      if (!(pcie_reset_flag = validReset())) {
        return -EINVAL;
      }
      break;

    case 's':
      if (!(pcie_link_speed_flag = validLinkSpeed())) {
        return -EINVAL;
      }
      break;

    case 'w':
      if (!(pcie_lane_width_flag = validLaneWidth())) {
        return -EINVAL;
      }
      break;

    case 't':
      if (!(tdp_level_flag = validTDPLevel())) {
        return -EINVAL;
      }
      break;

    case 'e':
      if (!(thresholds_flag = validThresholds())) {
        return -EINVAL;
      }
      break;

    case 'u':
      if (!(timeout_flag = validTimeout())) {
        return -EINVAL;
      }
      break;
    case 'i':
      if (!validPath()) {
        return -EINVAL;
      }
      break;
    case 'g':
      if (!getTraceBuffer()) {
        printTraceBuf(argv[0]);
        return -EINVAL;
      }
      return 0;
    case 'f':
      if (!(frequencies_flag = validFrequencies())) {
        return -EINVAL;
      }
      break;
    case 'v':
      if (!(voltage_flag = validVoltage())) {
        return -EINVAL;
      }
      break;
    case '?':
      printUsage(argv[0]);
      return -EINVAL;
    case 'V':
      printVersion();
      return 0;

    default:
      return -EINVAL;
    }
  }

  if (!cmd_flag && !code_flag) {
    DM_VLOG(HIGH) << "Aborting, must provide a command or code" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  if (!node_flag) {
    DM_VLOG(HIGH) << "Aborting, must provide a device node" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  if (!timeout_flag) {
    DM_VLOG(HIGH) << "Aborting, must provide a timeout value" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  return verifyService();
}
