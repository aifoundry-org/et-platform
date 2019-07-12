//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "PCIeDevice.h"

#include <experimental/filesystem>
#include <iostream>
#include <regex>
#include <string.h>

namespace fs = std::experimental::filesystem;

namespace et_runtime {
namespace device {

PCIeDevice::PCIeDevice(const std::string &path) : DeviceTarget(path) {}

std::vector<DeviceInformation> PCIeDevice::enumerateDevices() {
  std::vector<DeviceInformation> infos;
  /// We are going to match one of the character devices if it exists we are
  /// assuing the rest have been created
  const std::regex dev_name_re("et[[:digit:]]+bulk");
  for (auto &file : fs::directory_iterator("/dev")) {
    auto &path = file.path();
    if (fs::is_character_file(path)) {
      std::smatch match;
      auto path_str = path.string();
      if (std::regex_search(path_str, match, dev_name_re)) {
        // FIXME for now add empty information in the future
        // we should add real information
        DeviceInformation info;
        strncpy(info.name, path_str.c_str(), sizeof(info.name) - 1);
        infos.emplace_back(info);
      }
    }
  }
  return infos;
}

} // namespace device
} // namespace et_runtime
