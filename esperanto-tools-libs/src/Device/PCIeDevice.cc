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

#include "Support/Logging.h"

#include <absl/strings/str_format.h>
#include <experimental/filesystem>
#include <iostream>
#include <regex>
#include <string.h>

namespace fs = std::experimental::filesystem;

namespace et_runtime {
namespace device {

PCIeDevice::PCIeDevice(int index)
    : DeviceTarget(index),                                //
      index_(index),                                      //
      prefix_(absl::StrFormat("et%d", index)),            //
      bulk_("/dev/" + prefix_ + "bulk"),                  //
      from_mm_("/dev/" + prefix_ + "mb_from_mm"),         //
      to_mm_("/dev/" + prefix_ + "mb_to_mm"),             //
      from_sp_("/dev/" + prefix_ + "mb_from_sp"),         //
      to_sp_("/dev/" + prefix_ + "mb_to_sp"),             //
      pcie_userersr_("/dev/" + prefix_ + "pcie_useresr"), //
      trg_pcie_("/dev/" + prefix_ + "trg_pcie"),          //
      mbox_sp_("/dev/" + prefix_ + "mbox_sp"),            //
      mbox_mm_("/dev/" + prefix_ + "mbox_mm"),            //
      drct_dram_("/dev/" + prefix_ + "drct_dram")         //
{}

bool PCIeDevice::init() {
  // FIXME current we perform no initialization action that will not apply in
  // the future.
  RTINFO << "PCIeDevice performs no init actions";
  return true;
}

bool PCIeDevice::deinit() {
  assert(false);
  return false;
}

bool PCIeDevice::getStatus() {
  assert(false);
  return false;
}

DeviceInformation PCIeDevice::getStaticConfiguration() {
  assert(false);
  return {};
}

bool PCIeDevice::submitCommand() {
  assert(false);
  return false;
}

bool PCIeDevice::registerResponseCallback() {
  assert(false);
  return false;
}

bool PCIeDevice::registerDeviceEventCallback() {
  assert(false);
  return false;
}

bool PCIeDevice::defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) {
  // FIXME we perform no such action in the PCIE-device
  RTINFO << "PCIE-device performs no deviceDevMem";
  return true;
}

bool PCIeDevice::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  assert(false);
  return true;
}

bool PCIeDevice::writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) {
  return drct_dram_.write(dev_addr, buf, size);
}
bool PCIeDevice::launch(uintptr_t launch_pc, const layer_dynamic_info *params) {
  assert(false);
  return true;
}

bool PCIeDevice::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  assert(false);
  return true;
}

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
