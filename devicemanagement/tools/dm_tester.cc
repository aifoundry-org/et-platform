//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "../src/utils.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "deviceManagement/DeviceManagement.h"
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <device-layer/Autogen.h>
#include <device-layer/IDeviceLayer.h>
#include <dlfcn.h>
#include <experimental/filesystem>
#include <getopt.h>
#include <iostream>
#include <regex>
#include <unistd.h>
#include <glog/logging.h>

namespace fs = std::experimental::filesystem;

using namespace dev;
using namespace device_mgmt_api;
using namespace device_management;
using namespace std::chrono_literals;
using namespace boost::multiprecision;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

namespace {
constexpr int kIDevice = 0;
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

class DMLib {
public:
  DMLib() {
    handle_ = nullptr;
    handle_ = dlopen("libDM.so", RTLD_LAZY);

#ifdef TARGET_PCIE
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
#else
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
#endif
  }

  ~DMLib() {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }

  getDM_t getInstance() {
    const char *error;

    if (handle_) {
      getDM_t getDM = reinterpret_cast<getDM_t>(dlsym(handle_, "getInstance"));
      if (!(error = dlerror())) {
        return getDM;
      }
      DV_LOG(FATAL) << "error: " << error << std::endl;
    }
    return (getDM_t)0;
  }

  int verifyDMLib() {

    if (!(devLayer_.get()) || devLayer_.get() == nullptr) {
      DV_LOG(FATAL) << "Device Layer pointer is null!" << std::endl;
      return -EAGAIN;
    }

    dmi = getInstance();

    if (!dmi) {
      DV_LOG(FATAL) << "Device Management instance is null!" << std::endl;
      return -EAGAIN;
    }
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
static bool power_state_flag = false;

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

int runService(const char* input_buff, const uint32_t input_size, char* output_buff, const uint32_t output_size) {

  static DMLib dml;
  int ret;

  if (!(ret = dml.verifyDMLib())) {
    return ret;
  }
  DeviceManagement& dm = (*dml.dmi)(dml.devLayer_.get());

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  ret = dm.serviceRequest(node, code, input_buff, input_size, output_buff, output_size, hst_latency.get(),
                          dev_latency.get(), timeout);

  if (ret != DM_STATUS_SUCCESS) {
    DV_LOG(INFO) << "Service request failed with return code: " << ret << std::endl;
    return ret;
  }

  DV_LOG(INFO) << "Host Latency: " << *hst_latency << " ms" << std::endl;
  DV_LOG(INFO) << "Device Latency: " << *dev_latency << " us" << std::endl;
  DV_LOG(INFO) << "Service request succeeded" << std::endl;
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
    const uint32_t output_size = sizeof(asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    std::string str_output = std::string(output_buff, output_size);

    DV_LOG(INFO) << "Asset Output: " << str_output << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
    const uint32_t output_size = sizeof(asset_info_t);
    char output_buff[output_size] = {0};
    char pcie_speed[32];

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    switch (std::stoi(output_buff,nullptr,10)) {
      case 2:strncpy(pcie_speed, "PCIE_GEN1", sizeof(pcie_speed)); break;
      case 5:strncpy(pcie_speed, "PCIE_GEN2", sizeof(pcie_speed)); break;
      case 8:strncpy(pcie_speed, "PCIE_GEN3", sizeof(pcie_speed)); break;
      case 16:strncpy(pcie_speed, "PCIE_GEN4", sizeof(pcie_speed)); break;
      case 32:strncpy(pcie_speed, "PCIE_GEN5", sizeof(pcie_speed)); break;
    }


    DV_LOG(INFO) << "PCIE Speed: " << pcie_speed << std::endl;
  }break;

  case DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB: {
    const uint32_t output_size = sizeof(asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "DDR Memory Size: " << (std::stoi (output_buff,nullptr,10)) / 1024 << "GB" << std::endl;
  }break;

#ifdef TARGET_PCIE
  case DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION: {
    const uint32_t output_size = sizeof(asset_info_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
    
    try{
       std::string str_output = std::string(output_buff, output_size);
       DV_LOG(INFO) << "ASIC Revision: " << std::stoi (str_output,nullptr,16) << std::endl;
    }
    catch (const std::invalid_argument& ia) {
	     DV_LOG(INFO) << "Invalid response from device: " << ia.what() << '\n';
    }

  } break;
#endif
  case DM_CMD::DM_CMD_GET_MODULE_POWER_STATE: {
    const uint32_t output_size = sizeof(power_state_e);
    char output_buff[output_size] = {0};
    char power_state[32];

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    switch(POWER_STATE(*output_buff)) {
      case 0:
        strncpy(power_state,"POWER_STATE_FULL", sizeof(power_state));
        break;
      case 1:
        strncpy(power_state,"POWER_STATE_REDUCED", sizeof(power_state));
        break;
      case 2:
        strncpy(power_state,"POWER_STATE_LOWESET", sizeof(power_state));
        break;
      default:
        DV_LOG(INFO) << "Invalid power state: "  << std::endl;
        break;
    }

    DV_LOG(INFO) << "Power State Output: " << power_state << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_POWER_STATE: {
    if (!power_state_flag) {
      DV_LOG(FATAL) << "Aborting, --powerstate was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(power_state_e);
    const char input_buff[input_size] = {(char)power_state}; // bounds check prevents issues with narrowing

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
    DV_LOG(INFO) << "TDP Level Output: " << tdp_level << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL: {
    if (!tdp_level_flag) {
      DV_LOG(FATAL) << "Aborting, --tdplevel was not defined" << std::endl;
      return -EINVAL;
    }
    const uint32_t input_size = sizeof(uint8_t);
    const char input_buff[input_size] = 
                                {(char)tdp_level}; // bounds check prevents issues with narrowing

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
    DV_LOG(INFO) << "Low Temperature Threshold Output: " << +temperature_threshold->sw_temperature_c << " c"
                 << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS: {
    if (!thresholds_flag) {
      DV_LOG(FATAL) << "Aborting, --thresholds was not defined" << std::endl;
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

#ifdef TARGET_PCIE
  case DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE: {
    const uint32_t output_size = sizeof(current_temperature_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    current_temperature_t* cur_temp = (current_temperature_t*)output_buff;
    DV_LOG(INFO) << "Current Temperature Output: " << +cur_temp->temperature_c << " c" << std::endl;
  } break;
#endif

  case DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES: {
    const uint32_t output_size = sizeof(residency_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    residency_t* residency = (residency_t*)output_buff;
    DV_LOG(INFO) << "Residency Throttle States Output: " << residency->average << " us" << std::endl;
  } break;

#ifdef TARGET_PCIE
  case DM_CMD::DM_CMD_GET_MODULE_POWER: {
    const uint32_t output_size = sizeof(module_power_t);
    char output_buff[output_size] = {0};
    float power;

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_power_t* module_power = (module_power_t*)output_buff;
    power = (module_power->power >> 2) + (module_power->power & 0x03)*0.25;
    DV_LOG(INFO) << "Module Power Output: " << +power << " W" << std::endl;
  } break;
#endif

#ifdef TARGET_PCIE
  #define BIN2VOLTAGE(REG_VALUE, BASE, MULTIPLIER) \
     (BASE + REG_VALUE*MULTIPLIER)

  case DM_CMD::DM_CMD_GET_MODULE_VOLTAGE: {
    const uint32_t output_size = sizeof(module_voltage_t);
    char output_buff[output_size] = {0};
    uint32_t voltage;

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_voltage_t* module_voltage = (module_voltage_t*)output_buff;

    voltage = BIN2VOLTAGE(module_voltage->ddr, 250, 5);
    DV_LOG(INFO) << "Module Voltage DDR: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->l2_cache, 250, 5);
    DV_LOG(INFO) << "Module Voltage L2CACHE: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->maxion, 250, 5);
    DV_LOG(INFO) << "Module Voltage MAXION: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->minion, 250, 5);
    DV_LOG(INFO) << "Module Voltage MINION: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->pcie, 600, 6); //FIXME its 6.25 actualy, try float
    DV_LOG(INFO) << "Module Voltage PCIE: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->noc, 250, 5);
    DV_LOG(INFO) << "Module Voltage NOC: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->pcie_logic, 600, 6);
    DV_LOG(INFO) << "Module Voltage PCIE_LOGIC: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->vddqlp, 250, 10);
    DV_LOG(INFO) << "Module Voltage VDDQLP: " << +voltage << " mV" << std::endl;
    voltage = BIN2VOLTAGE(module_voltage->vddq, 250, 10);
    DV_LOG(INFO) << "Module Voltage VDDQ: " << +voltage << " mV" << std::endl;

  } break;
#endif

  case DM_CMD::DM_CMD_GET_MODULE_UPTIME: {
    const uint32_t output_size = sizeof(module_uptime_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    module_uptime_t* module_uptime = (module_uptime_t*)output_buff;
    DV_LOG(INFO) << "Module Uptime Output: " << module_uptime->day << " d " << +module_uptime->hours << " h "
                 << +module_uptime->mins << " m" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE: {
    const uint32_t output_size = sizeof(max_temperature_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_temperature_t* max_temperature = (max_temperature_t*)output_buff;
    DV_LOG(INFO) << "Module Max Temperature Output: " << +max_temperature->max_temperature_c << " c" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR: {
    const uint32_t output_size = sizeof(max_ecc_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_ecc_count_t* max_ecc_count = (max_ecc_count_t*)output_buff;
    DV_LOG(INFO) << "Max Memory Error Output: " << +max_ecc_count->count << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW: {
    const uint32_t output_size = sizeof(max_dram_bw_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    max_dram_bw_t* max_dram_bw = (max_dram_bw_t*)output_buff;
    DV_LOG(INFO) << "Module Max DDR BW Read Output: " << +max_dram_bw->max_bw_rd_req_sec << " GB/s"<<std::endl;
    DV_LOG(INFO) << "Module Max DDR BW Write Output: " << +max_dram_bw->max_bw_wr_req_sec <<" GB/s" <<std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES: {
    const uint32_t output_size = sizeof(residency_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    residency_t* residency = (residency_t*)output_buff;
    DV_LOG(INFO) << "Max throttle time Output: " << residency->average << " us" << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_DDR_ECC_COUNT:
  case DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT:
  case DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT: {
    if (!mem_count_flag) {
      DV_LOG(FATAL) << "Aborting, --memcount was not defined" << std::endl;
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
      DV_LOG(FATAL) << "Aborting, --pciereset was not defined" << std::endl;
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
    DV_LOG(INFO) << "Module PCIE ECC Output: " << +errors_count->ecc << std::endl;
    DV_LOG(INFO) << "Module PCIE UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER: {
    const uint32_t output_size = sizeof(dram_bw_counter_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    dram_bw_counter_t* dram_bw_counter = (dram_bw_counter_t*)output_buff;
    DV_LOG(INFO) << "Module DDR BW Read Counter Output: " << dram_bw_counter->bw_rd_req_sec << std::endl;
    DV_LOG(INFO) << "Module DDR BW Write Counter Output: " << dram_bw_counter->bw_wr_req_sec << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC: {
    const uint32_t output_size = sizeof(errors_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    errors_count_t* errors_count = (errors_count_t*)output_buff;
    DV_LOG(INFO) << "Module DDR ECC Output: " << +errors_count->ecc << std::endl;
    DV_LOG(INFO) << "Module DDR UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC: {
    const uint32_t output_size = sizeof(errors_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    errors_count_t* errors_count = (errors_count_t*)output_buff;
    DV_LOG(INFO) << "Module SRAM ECC Output: " << +errors_count->ecc << std::endl;
    DV_LOG(INFO) << "Module SRAM UECC Output: " << +errors_count->uecc << std::endl;
  } break;

  case DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED: {
    if (!pcie_link_speed_flag) {
      DV_LOG(FATAL) << "Aborting, --pciespeed was not defined" << std::endl;
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
      DV_LOG(FATAL) << "Aborting, --pciewidth was not defined" << std::endl;
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
    DV_LOG(INFO) << "ASIC Frequency Minion Shire: " << asic_frequencies->minion_shire_mhz << " Mhz" << std::endl;
    DV_LOG(INFO) << "ASIC Frequency NOC: " << asic_frequencies->noc_mhz << " Mhz" << std::endl;
    DV_LOG(INFO) << "ASIC Frequency Mem Shire:: " << asic_frequencies->mem_shire_mhz << " Mhz" << std::endl;
    DV_LOG(INFO) << "ASIC Frequency DDR: " << asic_frequencies->ddr_mhz << " Mhz" << std::endl;
    DV_LOG(INFO) << "ASIC Frequency PCIE Shire: " << asic_frequencies->pcie_shire_mhz << " Mhz" << std::endl;
    DV_LOG(INFO) << "ASIC Frequency IO Shire: " << asic_frequencies->io_shire_mhz << " Mhz" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH: {
    const uint32_t output_size = sizeof(dram_bw_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    dram_bw_t* dram_bw = (dram_bw_t*)output_buff;
    DV_LOG(INFO) << "DRAM Bandwidth Read Output: " << dram_bw->read_req_sec << " GB/s" << std::endl;
    DV_LOG(INFO) << "DRAM Bandwidth Write Output: " << dram_bw->write_req_sec << " GB/s" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION: {
    const uint32_t output_size = sizeof(percentage_cap_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    percentage_cap_t* percentage_cap = (percentage_cap_t*)output_buff;
    DV_LOG(INFO) << "DRAM Capacity Utilization Output: " << percentage_cap->pct_cap << " %" << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "ASIC per Core Datapath Utilization Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_UTILIZATION: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "ASIC Utilization Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_STALLS: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "ASIC Stalls Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_ASIC_LATENCY: {
    const uint32_t output_size = sizeof(uint8_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "ASIC Latency Output: " << +output_buff[0] << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MM_ERROR_COUNT: {
    const uint32_t output_size = sizeof(mm_error_count_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    mm_error_count_t* mm_error_count = (mm_error_count_t*)output_buff;
    DV_LOG(INFO) << "MM Hang Count: " << +mm_error_count->hang_count << std::endl;
    DV_LOG(INFO) << "MM Exception Count: " << +mm_error_count->exception_count << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS: {
    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }

    DV_LOG(INFO) << "Firmware Boot Status: Success! " << std::endl;
  } break;

  case DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS: {
    const uint32_t output_size = sizeof(device_mgmt_api::firmware_version_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
    
    device_mgmt_api::firmware_version_t* firmware_versions = (device_mgmt_api::firmware_version_t*)output_buff;

    uint32_t versions = firmware_versions->bl1_v;
    DV_LOG(INFO) << "BL1 Firmware versions: Major: " << (versions >> 24)
        << " Minor: " << (versions >> 16) << " Revision: " << (versions >> 8) << std::endl;
    versions = firmware_versions->bl2_v;
    DV_LOG(INFO) << "BL2 Firmware versions: Major: " << (versions >> 24)
        << " Minor: " << (versions >> 16) << " Revision: " << (versions >> 8) << std::endl;

    versions = firmware_versions->mm_v;
    DV_LOG(INFO) << "Master Minion Firmware versions: Major: " << (versions >> 24)
        << " Minor: " << (versions >> 16) << " Revision: " << (versions >> 8) << std::endl;

    versions = firmware_versions->wm_v;
    DV_LOG(INFO) << "Worker Minion versions: Major: " << (versions >> 24)
        << " Minor: " << (versions >> 16) << " Revision: " << (versions >> 8) << std::endl;

    versions = firmware_versions->machm_v;
    DV_LOG(INFO) << "Machine Minion versions: Major: " << (versions >> 24)
        << " Minor: " << (versions >> 16) << " Revision: " << (versions >> 8) << std::endl;  

  } break;          

  case DM_CMD::DM_CMD_SET_FIRMWARE_VERSION_COUNTER: {
    const uint32_t input_size = sizeof(uint256_t);
    const char input_buff[input_size] = {123}; //TODO: provide the counter by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break;

  case DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT: {
    const uint32_t input_size = sizeof(uint512_t);
    const char input_buff[input_size] = {123}; //TODO: provide the hash by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break; 

  case DM_CMD::DM_CMD_SET_SW_BOOT_ROOT_CERT: {
    const uint32_t input_size = sizeof(uint512_t);
    const char input_buff[input_size] = {123}; //TODO: provide the hash by argument

    const uint32_t output_size = sizeof(uint32_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(input_buff, input_size, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
  } break; 

  case DM_CMD::DM_CMD_GET_FUSED_PUBLIC_KEYS: {
    const uint32_t output_size = sizeof(uint256_t);
    char output_buff[output_size] = {0};

    if ((ret = runService(nullptr, 0, output_buff, output_size)) != DM_STATUS_SUCCESS) {
      return ret;
    }
    
    try{
       std::string str_output = std::string(output_buff, output_size);
       DV_LOG(INFO) << "Public keys: " << std::stoi (str_output,nullptr,16) << std::endl;
    }
    catch (const std::invalid_argument& ia) {
	     DV_LOG(INFO) << "Invalid response from device: " << ia.what() << '\n';
    }
  } break;
  
  default:
    DV_LOG(FATAL) << "Aborting, command: " << cmd << " (" << code << ") is currently unsupported" << std::endl;
    return -EINVAL;
    break;
  }
}

bool validDigitsOnly() {
  std::string str_optarg = std::string(optarg);
  std::regex re("^[0-9]+$");
  std::smatch m;
  if (!std::regex_search(str_optarg, m, re)) {
    DV_LOG(FATAL) << "Aborting, argument: " << str_optarg << " is not valid. It contains more than just digits"
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
    DV_LOG(FATAL) << "Aborting, command: " << str_optarg << " is not valid ( ^[a-zA-Z_]+$ )" << std::endl;
    return false;
  }

  auto it = commandCodeTable.find(str_optarg);

  if (it != commandCodeTable.end()) {
    cmd = it->first;
    code = it->second;
    return true;
  }

  DV_LOG(FATAL) << "Aborting, command: " << str_optarg << " not found" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, command: " << optarg << " is not valid" << std::endl;
    return false;
  }

  for (auto it = commandCodeTable.begin(); it != commandCodeTable.end(); ++it) {
    if (tmp_optarg == it->second) {
      cmd = it->first;
      code = it->second;
      return true;
    }
  }

  DV_LOG(FATAL) << "Aborting, command: " << optarg << " not found" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not valid ( 0-" << SCHAR_MAX << " )" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, node: " << str_optarg << " is not valid ( ^[0-5]{1}$ )" << std::endl;
    return false;
  }

  char* end;
  errno = 0;

  node = std::strtoul(optarg, &end, 10);

  if (node > 5 || end == optarg || *end != '\0' || errno != 0) {
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not a valid device node ( 0-5 )" << std::endl;
    return false;
  }

  return true;
}

bool validPowerState() {
  if (!validDigitsOnly()) {
    return false;
  }

  char* end;
  errno = 0;

  auto state = std::strtoul(optarg, &end, 10);

  if (state > 3 || end == optarg || *end != '\0' || errno != 0) {
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not a valid power state ( 0-3 )" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not a valid pcie lane width ( 0-1 )" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not a valid pcie link speed ( 0-1 )" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not a valid pcie reset type ( 0-2 )" << std::endl;
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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not valid tdp level ( 0-40 )" << std::endl;
    return false;
  }

  tdp_level = level;

  return true;
}

bool validThresholds() {
  std::string str_optarg = std::string(optarg);
  std::regex re("^[0-9]+,[0-9]+$");
  std::smatch m;
  if (!std::regex_search(str_optarg, m, re)) {
    DV_LOG(FATAL) << "Aborting, argument: " << str_optarg << " is not valid ( ^[0-9]+,[0-9]+$ )" << std::endl;
    return false;
  }

  std::size_t pos = str_optarg.find(",");

  char* end;
  errno = 0;

  auto lo = std::strtoul(str_optarg.substr(0, pos).c_str(), &end, 10);

  if (lo > SCHAR_MAX || end == optarg || *end != '\0' || errno != 0) {
    DV_LOG(FATAL) << "Aborting, argument: " << lo << " is not valid ( 0-" << SCHAR_MAX << " )" << std::endl;
    return false;
  }

  errno = 0;
  auto hi = std::strtoul(str_optarg.substr(pos + 1).c_str(), &end, 10);

  if (hi > SCHAR_MAX || end == optarg || *end != '\0' || errno != 0) {
    DV_LOG(FATAL) << "Aborting, argument: " << hi << " is not valid ( 0-" << SCHAR_MAX << " )" << std::endl;
    return false;
  }

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
    DV_LOG(FATAL) << "Aborting, argument: " << optarg << " is not valid ( 0 - " << ULONG_MAX << " )" << std::endl;
    return false;
  }

  return true;
}

static struct option long_options[] = {{"code", required_argument, 0, 'o'},
                                       {"command", required_argument, 0, 'm'},
                                       {"help", no_argument, 0, 'h'},
                                       {"memcount", required_argument, 0, 'c'},
                                       {"node", required_argument, 0, 'n'},
                                       {"powerstate", required_argument, 0, 'p'},
                                       {"pciereset", required_argument, 0, 'r'},
                                       {"pciespeed", required_argument, 0, 's'},
                                       {"pciewidth", required_argument, 0, 'w'},
                                       {"tdplevel", required_argument, 0, 't'},
                                       {"thresholds", required_argument, 0, 'e'},
                                       {"timeout", required_argument, 0, 'u'},
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

  for (auto const& [key, val] : commandCodeTable) {
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

void printPowerState(char* argv) {
  std::cout << std::endl;
  std::cout << "\t"
            << "-" << (char)long_options[5].val << ", --" << long_options[5].name << "=npower" << std::endl;
  std::cout << "\t\t"
            << "Set power state:" << std::endl;
  std::cout << std::endl;

  for (auto const& [key, val] : powerStateTable) {
    std::cout << "\t\t\t" << val << ": " << key << std::endl;
  }

  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " " << DM_CMD::DM_CMD_SET_MODULE_POWER_STATE
            << " -" << (char)long_options[5].val << " 0" << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_POWER_STATE"
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
            << "-" << (char)long_options[10].val << ", --" << long_options[10].name << "=nlotemp,nhitemp" << std::endl;
  std::cout << "\t\t"
            << "Set temperature thresholds (low,high)" << std::endl;
  std::cout << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[0].val << " "
            << DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS << " -" << (char)long_options[10].val << " 80,100"
            << std::endl;
  std::cout << "\t\t"
            << "Ex. " << argv << " -" << (char)long_options[1].val << " DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS"
            << " -" << (char)long_options[10].val << " 80,100" << std::endl;
}

void printUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "Usage: " << argv << " -o ncode | -m command [-n node] [-u nmsecs] [-h]"
            << "[-c ncount | -p npower | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nlotemp,nhitemp]"
            << std::endl;
  printCode(argv);
  printCommand(argv);
  printNode(argv);
  printTimeout(argv);
  printHelp(argv);
  printMemCount(argv);
  printPowerState(argv);
  printPCIEReset(argv);
  printPCIELinkSpeed(argv);
  printPCIELaneWidth(argv);
  printTDPLevel(argv);
  printThresholds(argv);
}

int main(int argc, char** argv) {
  int c;
  int option_index = 0;

  // Initialize Google's logging library.
  logging::LoggerDefault loggerDefault_;

  while (1) {

    c = getopt_long(argc, argv, "o:m:hc:n:p:r:s:w:t:e:u:", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {

    case 'o':
      if (cmd_flag) {
        DV_LOG(INFO) << "Command already provided, ignoring code: " << optarg << std::endl;
        break;
      }
      if (!(code_flag = validCode())) {
        return -EINVAL;
      }
      break;

    case 'm':
      if (code_flag) {
        DV_LOG(INFO) << "Code already provided, ignoring command: " << optarg << std::endl;
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
      if (!(power_state_flag = validPowerState())) {
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

    case '?':
      printUsage(argv[0]);
      return -EINVAL;

    default:
      return -EINVAL;
    }
  }

  if (!cmd_flag && !code_flag) {
    DV_LOG(FATAL) << "Aborting, must provide a command or code" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  if (!node_flag) {
    DV_LOG(FATAL) << "Aborting, must provide a device node" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  if (!timeout_flag) {
    DV_LOG(FATAL) << "Aborting, must provide a timeout value" << std::endl;
    printUsage(argv[0]);
    return -EINVAL;
  }

  return verifyService();
}
