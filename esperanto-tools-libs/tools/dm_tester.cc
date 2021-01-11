//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/DeviceManagement/DeviceManagement.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"

#include "absl/flags/marshalling.h"
#include "absl/strings/string_view.h"
#include <absl/flags/flag.h>
#include <errno.h>
#include <dlfcn.h>
#include <iostream>
#include <regex>

using namespace device_management;

struct libdm {
  libdm() { handle_ = dlopen("libDM.so", RTLD_LAZY); }
  ~libdm() { dlclose(handle_); }

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

struct commandString {
  explicit commandString(std::string c = "") : cmd(c), cmd_id(0) {
    auto it = commandCodeTable.find(cmd);
    if (it != commandCodeTable.end()) {
      cmd_id = it->second;
    }
  }

  std::string cmd;
  uint32_t cmd_id;
};

static std::string AbslUnparseFlag(commandString c) {
  return absl::UnparseFlag(c.cmd);
}

static bool AbslParseFlag(absl::string_view text, commandString *c,
                          std::string *error) {
  if (!absl::ParseFlag(text, &c->cmd, error)) {
    return false;
  }

  auto it = commandCodeTable.find(c->cmd);
  if (it == commandCodeTable.end()) {
    *error = "command code is not valid!\n";
    return false;
  }
  c->cmd_id = it->second;

  return true;
}
ABSL_FLAG(commandString, cmd, commandString("DM_CMD_GET_MODULE_MANUFACTURE_NAME"),
          "set device management command code");

struct deviceString {
  explicit deviceString(std::string d = "") : dev(d) {}
  std::string dev;
};

static std::string AbslUnparseFlag(deviceString d) {
  return absl::UnparseFlag(d.dev);
}

static bool AbslParseFlag(absl::string_view text, deviceString *d,
                          std::string *error) {
  if (!absl::ParseFlag(text, &d->dev, error)) {
    return false;
  }

  std::regex re("^et[0-5]{1}_(?=mgmt$|ops$)");
  std::smatch m;
  if (!std::regex_search(d->dev, m, re)) {
    *error = "device node is not valid!\n";
    return false;
  }

  return true;
}
ABSL_FLAG(deviceString, dev, deviceString("et0_mgmt"),
          "set device management device node");

struct powerStateString {
  explicit powerStateString(std::string s = "")
    : state(s)
    , state_id(0) {
    auto it = powerStateTable.find(state);
    if (it != powerStateTable.end()) {
      state_id = it->second;
    }
  }

  std::string state;
  uint32_t state_id;
};

static std::string AbslUnparseFlag(powerStateString s) {
  return absl::UnparseFlag(s.state);
}

static bool AbslParseFlag(absl::string_view text, powerStateString* s, std::string* error) {
  if (!absl::ParseFlag(text, &s->state, error)) {
    return false;
  }

  auto it = powerStateTable.find(s->state);
  if (it == powerStateTable.end()) {
    *error = "Module Power State mode is not valid!\n";
    return false;
  }
  s->state_id = it->second;

  return true;
}
ABSL_FLAG(powerStateString, pss, powerStateString("POWER_STATE_FULL"), "set module power state mode");

struct TDPString {
  explicit TDPString(std::string s = "")
    : tdp(s)
    , tdp_id(0) {
    auto it = TDPLevelTable.find(tdp);
    if (it != TDPLevelTable.end()) {
      tdp_id = it->second;
    }
  }

  std::string tdp;
  uint32_t tdp_id;
};

static std::string AbslUnparseFlag(TDPString s) {
  return absl::UnparseFlag(s.tdp);
}

static bool AbslParseFlag(absl::string_view text, TDPString* s, std::string* error) {
  if (!absl::ParseFlag(text, &s->tdp, error)) {
    return false;
  }

  auto it = TDPLevelTable.find(s->tdp);
  if (it == TDPLevelTable.end()) {
    *error = "Static TDP level is not valid!\n";
    return false;
  }
  s->tdp_id = it->second;

  return true;
}
ABSL_FLAG(TDPString, tdps, TDPString("TDP_LEVEL_ONE"), "set static TDP level");

struct pcieResetString {
  explicit pcieResetString(std::string r = "")
    : reset(r)
    , reset_id(0) {
    auto it = pcieResetTable.find(reset);
    if (it != pcieResetTable.end()) {
      reset_id = it->second;
    }
  }

  std::string reset;
  uint32_t reset_id;
};

static std::string AbslUnparseFlag(pcieResetString r) {
  return absl::UnparseFlag(r.reset);
}

static bool AbslParseFlag(absl::string_view text, pcieResetString* r, std::string* error) {
  if (!absl::ParseFlag(text, &r->reset, error)) {
    return false;
  }

  auto it = pcieResetTable.find(r->reset);
  if (it == pcieResetTable.end()) {
    *error = "PCIE reset level is not valid!\n";
    return false;
  }
  r->reset_id = it->second;

  return true;
}
ABSL_FLAG(pcieResetString, pcieReset, pcieResetString("PCIE_RESET_FLR"), "Set PCIE reset level");

struct pcieLinkSpeedString {
  explicit pcieLinkSpeedString(std::string s = "")
    : speed(s)
    , speed_id(0) {
    auto it = pcieLinkSpeedTable.find(speed);
    if (it != pcieLinkSpeedTable.end()) {
      speed_id = it->second;
    }
  }

  std::string speed;
  uint32_t speed_id;
};

static std::string AbslUnparseFlag(pcieLinkSpeedString s) {
  return absl::UnparseFlag(s.speed);
}

static bool AbslParseFlag(absl::string_view text, pcieLinkSpeedString* s, std::string* error) {
  if (!absl::ParseFlag(text, &s->speed, error)) {
    return false;
  }

  auto it = pcieLinkSpeedTable.find(s->speed);
  if (it == pcieLinkSpeedTable.end()) {
    *error = "PCIE link speed is not valid!\n";
    return false;
  }
  s->speed_id = it->second;

  return true;
}
ABSL_FLAG(pcieLinkSpeedString, pcieLinkSpeed, pcieLinkSpeedString("PCIE_LINK_SPEED_GEN3"), "Set PCIE link speed");

struct pcieLaneWidthString {
  explicit pcieLaneWidthString(std::string w = "")
    : width(w)
    , width_id(0) {
    auto it = pcieLaneWidthTable.find(width);
    if (it != pcieLaneWidthTable.end()) {
      width_id = it->second;
    }
  }

  std::string width;
  uint32_t width_id;
};

static std::string AbslUnparseFlag(pcieLaneWidthString w) {
  return absl::UnparseFlag(w.width);
}

static bool AbslParseFlag(absl::string_view text, pcieLaneWidthString* w, std::string* error) {
  if (!absl::ParseFlag(text, &w->width, error)) {
    return false;
  }

  auto it = pcieLaneWidthTable.find(w->width);
  if (it == pcieLaneWidthTable.end()) {
    *error = "PCIE lane width is not valid!\n";
    return false;
  }
  w->width_id = it->second;

  return true;
}
ABSL_FLAG(pcieLaneWidthString, pcieLaneWidth, pcieLaneWidthString("PCIE_LANE_W_SPLIT_x4"), "Set PCIE lane width");

ABSL_FLAG(uint32_t, timeout, 0, "Set timeout duration in msec: 0 = disabled");
ABSL_FLAG(uint32_t, count, 0, "Set count value");

ABSL_FLAG(uint16_t, tholdLow, 0, "Set module low temperature threshold in Celsius");
ABSL_FLAG(uint16_t, tholdHigh, 0, "Set module high temperature threshold in Celsius");

ABSL_FLAG(std::string, path, "", "Set path to load file");

int testAsset(DeviceManagement &dm, const char *device_node, uint32_t cmd_id,
              const char *input_buff, const uint32_t input_size,
              const uint32_t output_size, uint32_t timeout) {
  char output_buff[output_size];
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  dm.serviceRequest(device_node, cmd_id, input_buff, input_size, output_buff,
                    output_size, hst_latency.get(), dev_latency.get(), timeout);

  std::printf("Output: %.*s\n", output_size, output_buff);
  std::cout << "Host Latency: " << *hst_latency << std::endl;
  std::cout << "Device Latency: " << *dev_latency << std::endl;

  return 0;
}

int main(int argc, char *argv[]) {
  et_runtime::ParseCommandLineOptions(argc, argv);

  libdm dml;
  getDM_t dmi = dml.getInstance();

  if (!dmi) {
    return -EAGAIN;
  }

  DeviceManagement &dm = (*dmi)();

  commandString cs = absl::GetFlag(FLAGS_cmd);
  deviceString ds = absl::GetFlag(FLAGS_dev);
  uint32_t timeout = absl::GetFlag(FLAGS_timeout);
  uint32_t count = absl::GetFlag(FLAGS_count);
  std::string path = absl::GetFlag(FLAGS_path);
  powerStateString pss = absl::GetFlag(FLAGS_pss);
  TDPString tdps = absl::GetFlag(FLAGS_tdps);
  uint8_t thLow = static_cast<uint8_t>(absl::GetFlag(FLAGS_tholdLow));
  uint8_t thHigh = static_cast<uint8_t>(absl::GetFlag(FLAGS_tholdHigh));
  pcieResetString pcieReset = absl::GetFlag(FLAGS_pcieReset);
  pcieLinkSpeedString pcieSpeed = absl::GetFlag(FLAGS_pcieLinkSpeed);
  pcieLaneWidthString pcieWidth = absl::GetFlag(FLAGS_pcieLaneWidth);

  std::cout << "Command code: " << cs.cmd << " or " << cs.cmd_id << std::endl;
  std::cout << "Device node: " << ds.dev << std::endl;
  std::cout << "timeout: " << timeout << std::endl;

  switch (cs.cmd_id) {
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER_STATE:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_UPTIME:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MM_THREADS_STATE:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 4, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DRIVER_REVISION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ADDR:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_FUSED_PUBLIC_KEYS:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_THROTTLE_TIME:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS:
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 20, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 24, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_POWER_STATE:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(pss.state_id), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(tdps.tdp_id), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RESET:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(pcieReset.reset_id), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(pcieSpeed.speed_id), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(pcieWidth.width_id), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RETRAIN_PHY:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(0), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_RESET_ETSOC:
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(0), 4, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE:
  case device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT:
  case device_mgmt_api::DM_CMD::DM_CMD_SET_SW_BOOT_ROOT_CERT:
    if (path.empty()) {
      std::cout << "--path not provided or invalid" << std::endl;
      return -EINVAL;
    }
    testAsset(dm, ds.dev.c_str(), cs.cmd_id, path.data(), 1, 8, timeout);
    break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS: {
    if (!thLow) {
      std::cout << "--tholdLow not provided or invalid" << std::endl;
      return -EINVAL;
    }

    if (!thHigh) {
      std::cout << "--tholdHigh not provided or invalid" << std::endl;
      return -EINVAL;
    }

    device_mgmt_api::temperature_threshold_t tholds{.lo_temperature_c = thLow, .hi_temperature_c = thHigh};

    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(&tholds), 8, 8, timeout);
  } break;
  case device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT:
  case device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT:
  case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT:
    if (!count) {
      std::cout << "--count not provided or invalid" << std::endl;
      return -EINVAL;
    }

    testAsset(dm, ds.dev.c_str(), cs.cmd_id, reinterpret_cast<char*>(count), 4, 8, timeout);
  default:
    return -EINVAL;
    break;
  }

  return 0;
}
