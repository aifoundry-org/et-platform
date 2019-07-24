//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "DeviceAPI/Command.h"

#include "Core/Device.h"
#include "DeviceAPI/Commands.h"
#include "Support/Logging.h"
#include "Support/STLHelpers.h"
#include "etrt-bin.h"

#define INCLUDE_FOR_HOST
#include "../etlibdevice.h"
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
  target_device.readDevMem((uintptr_t)srcDevPtr, count, dstHostPtr);
  setResponse(ReadResponse());
  return etrtSuccess;
}

etrtError WriteCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();

  target_device.writeDevMem((uintptr_t)dstDevPtr, count, srcHostPtr);
  setResponse(WriteResponse());
  return etrtSuccess;
}

etrtError LaunchCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  const auto *params = (const et_runtime::device::layer_dynamic_info *)args_buff.data();

  fprintf(stderr,
          "LaunchCommand::Going to execute kernel {0x%lx} %s [%s]\n"
          "  tensor_a = 0x%" PRIx64 "\n"
          "  tensor_b = 0x%" PRIx64 "\n"
          "  tensor_c = 0x%" PRIx64 "\n"
          "  tensor_d = 0x%" PRIx64 "\n"
          "  tensor_e = 0x%" PRIx64 "\n"
          "  tensor_f = 0x%" PRIx64 "\n"
          "  tensor_g = 0x%" PRIx64 "\n"
          "  tensor_h = 0x%" PRIx64 "\n"
          "  pc/id    = 0x%" PRIx64 "\n",
          kernel_pc, kernel_name.c_str(), demangle(kernel_name).c_str(),
          params->tensor_a, params->tensor_b, params->tensor_c,
          params->tensor_d, params->tensor_e, params->tensor_f,
          params->tensor_g, params->tensor_h, params->kernel_id);

  target_device.writeDevMem(RT_HOST_KERNEL_LAUNCH_INFO, sizeof(*params), params);
  target_device.launch(kernel_pc, params); // ETSOC backend - JIT, not covered by b4c

  setResponse(LaunchResponse());
  return etrtSuccess;
}

} // namespace device_api
} // namespace et_runtime
