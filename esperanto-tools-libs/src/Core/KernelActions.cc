//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "KernelActions.h"

#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

#include "esperanto/runtime/CodeManagement/Kernel.h"
namespace et_runtime {
ErrorOr<::device_api::non_privileged::DEV_API_KERNEL_STATE>
KernelActions::state(Stream *stream) const {

  auto state_cmd = std::make_shared<device_api::devfw_commands::KernelStateCmd>(
      stream->id(), id_);
  stream->addCommand(state_cmd);

  auto rsp_future = state_cmd->getFuture();
  auto response = rsp_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_STATE_RSP);
  return static_cast<::device_api::non_privileged::DEV_API_KERNEL_STATE>(
      response.status);
}

ErrorOr<::device_api::non_privileged::DEV_API_KERNEL_ABORT_RESPONSE_RESULT>
KernelActions::abort(Stream *stream) const {

  auto abort_cmd = std::make_shared<device_api::devfw_commands::KernelAbortCmd>(
      stream->id(), id_);
  stream->addCommand(abort_cmd);

  auto rsp_future = abort_cmd->getFuture();
  auto response = rsp_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_ABORT_RSP);
  return static_cast<
      ::device_api::non_privileged::DEV_API_KERNEL_ABORT_RESPONSE_RESULT>(
      response.status);
}

} // namespace et_runtime
