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
#include <dlfcn.h>
#include <iostream>
#include <regex>

using namespace device_management;

struct libdm {
  libdm() { handle_ = dlopen("libDM.so", RTLD_LAZY); }
  ~libdm() { dlclose(&handle_); }

  DeviceManagement &getInstance() {
    const char *error;

    if (!handle_) {
      throw "Could not load library!";
    }

    getDM_t getDM = reinterpret_cast<getDM_t>(dlsym(handle_, "getInstance"));

    if ((error = dlerror())) {
      throw "Could not load symbol!";
    }

    return (*getDM)();
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

  std::regex re("^et[0-9]+_(?=mgmt$|ops$)");
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

int testAsset(DeviceManagement &dm, const char *device_node, uint32_t cmd_id,
              const char *input_buff, const uint32_t input_size,
              const uint32_t output_size, uint32_t timeout) {
  char output_buff[output_size];
  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint32_t>();

  dm.serviceRequest(device_node, cmd_id, input_buff, input_size, output_buff,
                    output_size, hst_latency.get(), dev_latency.get(), timeout);

  std::printf("Output: %s\n", output_buff);
  std::cout << "Host Latency: " << *hst_latency << std::endl;
  std::cout << "Device Latency: " << *dev_latency << std::endl;

  return 0;
}

int main(int argc, char *argv[]) {
  et_runtime::ParseCommandLineOptions(argc, argv);

  try {
    libdm dml;
    DeviceManagement &dm = dml.getInstance();

    commandString cs = absl::GetFlag(FLAGS_cmd);
    deviceString ds = absl::GetFlag(FLAGS_dev);
    uint32_t timeout = absl::GetFlag(FLAGS_timeout);

    std::cout << "Command code: " << cs.cmd << " or " << cs.cmd_id << std::endl;
    std::cout << "Device node: " << ds.dev << std::endl;
    std::cout << "timeout: " << timeout << std::endl;

    switch (cs.cmd_id) {
      case 0x01:
      case 0x04:
      case 0x09:
        testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 4, timeout);
        break;
      case 0x00:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x02:
      case 0x03:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x05:
      case 0x06:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x07:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x08:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x0A:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x0B:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x0C:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x0D:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
      case 0x0E:  // Awaiting parameter size informationf from ET. For now, using DUMMY VALUE of 8 BYTES
        testAsset(dm, ds.dev.c_str(), cs.cmd_id, nullptr, 0, 8, timeout);
        break;
      default:
        throw "Unhandled cmd_id!";
        break;
    }

    return 0;
  } catch (const char *msg) {
      std::cerr << msg << std::endl;
      return 1;
  }
}
