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
ABSL_FLAG(commandString, cmd, commandString("GET_MODULE_MANUFACTURE_NAME"),
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

ABSL_FLAG(uint32_t, timeout, 0, "Set timeout duration in msec: 0 = disabled");

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
  std::string path = absl::GetFlag(FLAGS_path);

  std::cout << "Command code: " << cs.cmd << " or " << cs.cmd_id << std::endl;
  std::cout << "Device node: " << ds.dev << std::endl;
  std::cout << "timeout: " << timeout << std::endl;

  switch (cs.cmd_id) {
    case CommandCode::GET_MODULE_MANUFACTURE_NAME:
    case CommandCode::GET_MODULE_PART_NUMBER:
    case CommandCode::GET_MODULE_SERIAL_NUMBER:
    case CommandCode::GET_ASIC_CHIP_REVISION:
    case CommandCode::GET_MODULE_DRIVER_REVISION:
    case CommandCode::GET_MODULE_PCIE_ADDR:
    case CommandCode::GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
    case CommandCode::GET_MODULE_MEMORY_SIZE_MB:
    case CommandCode::GET_MODULE_REVISION:
    case CommandCode::GET_MODULE_FORM_FACTOR:
    case CommandCode::GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
    case CommandCode::GET_MODULE_MEMORY_TYPE:
    case CommandCode::GET_FUSED_PUBLIC_KEYS:
    case CommandCode::GET_FIRMWARE_BOOT_STATUS:
      testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 8, timeout);
      break;
    case CommandCode::GET_MODULE_FIRMWARE_REVISIONS:
      testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 20, timeout);
      break;
    case CommandCode::SET_FIRMWARE_UPDATE:
    case CommandCode::SET_SP_BOOT_ROOT_CERT:
    case CommandCode::SET_SW_BOOT_ROOT_CERT:
      if (path.empty()) {
        std::cout << "--path not provided or invalid" << std::endl;
        return -EINVAL;
      }
      testAsset(dm, ds.dev.c_str(), cs.cmd_id, path.data(), 1, 8, timeout);
      break;
    default:
      return -EINVAL;
      break;
  }

  return 0;
}
