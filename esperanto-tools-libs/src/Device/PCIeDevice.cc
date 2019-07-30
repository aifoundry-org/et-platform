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

#if ENABLE_DEVICE_FW
#include "host_message.h"
#endif // ENABLE_DEVICE_FW

#include <absl/strings/str_format.h>
#include <experimental/filesystem>
#include <iostream>
#include <regex>
#include <string.h>

namespace fs = std::experimental::filesystem;

namespace et_runtime {
namespace device {

PCIeDevice::PCIeDevice(int index)
    : DeviceTarget(index),                                       //
      index_(index),                                             //
      prefix_(absl::StrFormat("et%d", index)),                   //
      bulk_("/dev/" + prefix_ + "bulk"),                         //
      drct_dram_("/dev/" + prefix_ + "drct_dram", 0x8100000000), //
      mm_("/dev/" + prefix_ + "mb_mm"),                          //
      sp_("/dev/" + prefix_ + "mb_sp"),                          //
      pcie_userersr_("/dev/" + prefix_ + "pcie_useresr"),        //
      r_mbox_sp_("/dev/" + prefix_ + "r_mbox_sp"),               //
      r_mbox_mm_("/dev/" + prefix_ + "r_mbox_mm"),               //
      trg_pcie_("/dev/" + prefix_ + "trg_pcie")                  //
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

bool PCIeDevice::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  return drct_dram_.read(dev_addr, buf, size);
}

bool PCIeDevice::writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) {
  return drct_dram_.write(dev_addr, buf, size);
}

bool PCIeDevice::launch(uintptr_t launch_pc, const layer_dynamic_info *params) {
#if ENABLE_DEVICE_FW
  // FIXME ugly hack this needsOB to be cleaned up
  struct __attribute__((__packed__)) launch_message_t {
    uint64_t mbox_message_id_t;
    layer_dynamic_info params;
    kernel_info_t kernel_info;
  };

  launch_message_t msg = {0};

  msg.mbox_message_id_t = MBOX_MESSAGE_ID_KERNEL_LAUNCH;
  msg.params = *params;
  msg.kernel_info.compute_pc = launch_pc;
  msg.kernel_info.shire_mask = 0x1;
  msg.kernel_info.kernel_params_ptr = 0;
  msg.kernel_info.grid_config_ptr = 0;

  auto res = mm_.write(&msg, sizeof(msg));
  assert(res);

  std::array<uint8_t, MailBoxDev::MBOX_MAX_MESSAGE_LENGTH> message = {0};
  auto size =
      mm_.read(message.data(), message.size(), std::chrono::seconds(60));
  assert(size == sizeof(devfw_response_t));
  auto response = reinterpret_cast<devfw_response_t *>(message.data());
  RTDEBUG << "MessageID: " << response->message_id
          << " kernel_id: " << response->kernel_id
          << " kernel_result: " << response->launch_response << "\n";

  if (response->message_id == MBOX_MESSAGE_ID_KERNEL_LAUNCH_RESPONSE &&
      response->launch_response == MBOX_KERNEL_LAUNCH_RESPONSE_RESULT_OK) {
    RTDEBUG << "Received successfull launch \n";
  } else {
    assert(false);
  }

#endif // ENABLE_DEVICE_FW
  return true;
}

bool PCIeDevice::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  assert(false);
  return true;
}

uintptr_t PCIeDevice::dramBaseAddr() const { return drct_dram_.baseAddr(); }

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
