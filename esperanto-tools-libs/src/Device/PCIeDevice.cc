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

#include "Core/CommandLineOptions.h"
#include "Device/DeviceFwTypes.h"
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
    : DeviceTarget(index),                     //
      index_(index),                           //
      prefix_(absl::StrFormat("et%d", index)), //
      bulk_("/dev/" + prefix_ + "bulk"),       //
      mm_("/dev/" + prefix_ + "mb_mm"),        //
      sp_("/dev/" + prefix_ + "mb_sp")         //
{}

bool PCIeDevice::init() {
  // FIXME current we perform no initialization action that will not apply in
  // the future.
  RTINFO << "PCIeDevice: Initialization \n";
  auto res = mm_.reset();
  assert(res);
  RTINFO << "PCIEDevice: Reset MM mailbox \n";
  // Wait for the device to be ready
  // FIXME "random" wait time and polling interval
  auto wait_time = std::chrono::seconds(2 * 60);
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = wait_time / 10;
  bool mb_ready = mm_.ready();
  while (mb_ready == 0) {
    std::this_thread::sleep_for(polling_interval);
    if (end < Clock::now()) {
      RTERROR << "Mailbox not ready in time \n";
      return false;
    }
    mb_ready = mm_.ready();
  }

  RTINFO << "PCIEDevice: MM mailbox ready \n";
  device_alive_ = true;
  return true;
}

bool PCIeDevice::deinit() {
  return true;
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

bool PCIeDevice::readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) {
  return bulk_.read_mmio(dev_addr, buf, size);
}

bool PCIeDevice::writeDevMemMMIO(uintptr_t dev_addr, size_t size,
                                 const void *buf) {
  return bulk_.write_mmio(dev_addr, buf, size);
}

bool PCIeDevice::readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) {
  return bulk_.read_dma(dev_addr, buf, size);
}

bool PCIeDevice::writeDevMemDMA(uintptr_t dev_addr, size_t size,
                                const void *buf) {
  return bulk_.write_dma(dev_addr, buf, size);
}

bool PCIeDevice::mb_write(const void *data, ssize_t size) {
  return mm_.write(data, size);
}

ssize_t PCIeDevice::mb_read(void *data, ssize_t size, TimeDuration wait_time) {
  return mm_.read(data, size, wait_time);
}

bool PCIeDevice::launch(uintptr_t launch_pc, const layer_dynamic_info *params) {
  abort();
  return true;
}

bool PCIeDevice::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  assert(false);
  return true;
}

uintptr_t PCIeDevice::dramBaseAddr() const { return bulk_.baseAddr(); }

uintptr_t PCIeDevice::dramSize() const { return bulk_.size(); }

ssize_t PCIeDevice::mboxMsgMaxSize() const { return mm_.mboxMaxMsgSize(); }

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
