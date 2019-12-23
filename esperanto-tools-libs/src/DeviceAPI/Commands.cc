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
#include "DeviceAPI/CommandsGen.h"

#include "DeviceAPI/EventsGen.h"

#include "Core/DeviceFwTypes.h"
#include "PCIEDevice/MailBoxDev.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"
#include "esperanto/runtime/Support/STLHelpers.h"

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

#include "demangle.h"

#include <esperanto/device-api/device_api.h>

using namespace et_runtime;

namespace et_runtime {
namespace device_api {

CommandID CommandBase::global_command_id_ = 0;

CommandBase::CommandBase() : id_(global_command_id_++) {}

void CommandBase::registerStream(Stream *stream) { stream_ = stream; }

namespace pcie_commands {

etrtError ReadCommand::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  // FIXME this heuristic should be revisited
  uint64_t threshold_size = 1 << 12;
  if (count < threshold_size) {
    target_device.readDevMemMMIO((uintptr_t)srcDevPtr, count, dstHostPtr);
  } else {
    target_device.readDevMemDMA((uintptr_t)srcDevPtr, count, dstHostPtr);
  }
  setResponse(pcie_responses::ReadResponse());
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
  setResponse(pcie_responses::WriteResponse());
  return etrtSuccess;
}

} // namespace pcie_commands

} // namespace device_api
} // namespace et_runtime
