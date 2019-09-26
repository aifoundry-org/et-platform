//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "DeviceAPI/Commands.h"

#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Device/DeviceFwTypes.h"
#include "Device/MailBoxDev.h"
#include "Support/Logging.h"
#include "Support/STLHelpers.h"

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

#include "demangle.h"

using namespace et_runtime;

namespace et_runtime {
namespace device_api {

CommandBase::IDty CommandBase::command_id_ = 0;

CommandBase::CommandBase() { command_id_++; }

etrtError ConfigureCommand::execute(Device *device) {
  auto status = device->loadFirmwareOnDevice();
  assert(status == etrtSuccess);
  setResponse(ConfigureResponse());
  return etrtSuccess;
}

etrtError ReadCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  // FIXME this heuristic should be revisited
  uint64_t threshold_size = 1 << 12;
  if (count < threshold_size) {
    target_device.readDevMemMMIO((uintptr_t)srcDevPtr, count, dstHostPtr);
  } else {
    target_device.readDevMemDMA((uintptr_t)srcDevPtr, count, dstHostPtr);
  }
  setResponse(ReadResponse());
  return etrtSuccess;
}

etrtError WriteCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  // FIXME this heuristic should be revisited
  uint64_t threshold_size = 1 << 12;
  if ( count < threshold_size) {
    target_device.writeDevMemMMIO((uintptr_t)dstDevPtr, count, srcHostPtr);
  } else {
      target_device.writeDevMemDMA((uintptr_t)dstDevPtr, count, srcHostPtr);
  }
  setResponse(WriteResponse());
  return etrtSuccess;
}

etrtError LaunchCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  const auto *params = (const et_runtime::device::layer_dynamic_info *)args_buff.data();

  RTDEBUG << "LaunchCommand::Going to execute kernel {0x" << std::hex << kernel_pc
          << "} " << kernel_name << " [" << demangle(kernel_name) << "] \n"
          << " tensor_a = 0x" << params->tensor_a << "\n"
          << " tensor_b = 0x" << params->tensor_b << "\n"
          << " tensor_c = 0x" << params->tensor_c << "\n"
          << " tensor_d = 0x" << params->tensor_d << "\n"
          << " tensor_e = 0x" << params->tensor_e << "\n"
          << " tensor_f = 0x" << params->tensor_f << "\n"
          << " tensor_g = 0x" << params->tensor_g << "\n"
          << " tensor_h = 0x" << params->tensor_h << "\n"
          << " pc/id    = 0x" << params->kernel_id << "\n";

#if ENABLE_DEVICE_FW
  device_fw::host_message_t msg = {0};

  // FIXME we should be querying the device-fw for that information first
  auto active_shires_opt = absl::GetFlag(FLAGS_shires);
  int active_shires = std::stoi(active_shires_opt);

  msg.message_id = device_fw::MBOX_MESSAGE_ID_KERNEL_LAUNCH;
  msg.kernel_params =
      *reinterpret_cast<const device_fw::kernel_params_t *>(params);
  msg.kernel_info.compute_pc = kernel_pc;
  msg.kernel_info.shire_mask = (1ULL << active_shires) - 1;
  msg.kernel_info.kernel_params_ptr = 0;
  msg.kernel_info.grid_config_ptr = 0;

  auto res = target_device.mb_write(&msg, sizeof(msg));
  assert(res);

  std::vector<uint8_t> message(target_device.mboxMsgMaxSize(), 0);
  auto size = target_device.mb_read(message.data(), message.size(),
                                    std::chrono::seconds(60));
  assert(size == sizeof(device_fw::devfw_response_t));
  auto response =
      reinterpret_cast<device_fw::devfw_response_t *>(message.data());
  RTDEBUG << "MessageID: " << response->message_id
          << " kernel_id: " << response->kernel_id
          << " kernel_result: " << response->response_id << "\n";

  if (response->message_id == device_fw::MBOX_MESSAGE_ID_KERNEL_RESULT &&
      response->response_id == device_fw::MBOX_KERNEL_RESULT_OK) {
    RTDEBUG << "Received successfull launch \n";
  } else {
    assert(false);
  }

#else
  struct {
    uint64_t unused;
    uint64_t kernel_pc;
    et_runtime::device::layer_dynamic_info params;
  } __attribute__((packed)) fake_fw_launch_info;

  memset(&fake_fw_launch_info, 0, sizeof(fake_fw_launch_info));
  fake_fw_launch_info.kernel_pc = kernel_pc;
  fake_fw_launch_info.params = *params;

  target_device.writeDevMemMMIO(RT_HOST_KERNEL_LAUNCH_INFO,
                                sizeof(fake_fw_launch_info),
                                &fake_fw_launch_info);
  target_device.launch();
#endif // ENABLE_DEVICE_FW
  setResponse(LaunchResponse());
  return etrtSuccess;
}

} // namespace device_api
} // namespace et_runtime
