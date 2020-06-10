//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/DeviceHelpers.h"

#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

#include <memory>
#include <string>

namespace et_runtime {

template <class SetLogLevelCmd, class SetLogLevelRsp>
static bool set_log_level_helper(Device &dev,
                                 ::device_api::LOG_LEVELS log_level) {

  auto log_level_cmd = std::make_shared<SetLogLevelCmd>(0, log_level);

  dev.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  // FIXME the command type should be a constant of the response class
  assert(response.response_info.message_id ==
             ::device_api::MBOX_DEVAPI_MESSAGE_ID_SET_MASTER_LOG_LEVEL_RSP ||
         response.response_info.message_id ==
             ::device_api::MBOX_DEVAPI_MESSAGE_ID_SET_WORKER_LOG_LEVEL_RSP);
  return static_cast<bool>(response.status);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_critical() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_CRITICAL);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_error() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_ERROR);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_warning() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_WARNING);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_info() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_INFO);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_debug() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_DEBUG);
}

template <class SetLogLevelCmd, class SetLogLevelRsp>
bool SetLogLevel<SetLogLevelCmd, SetLogLevelRsp>::set_level_trace() {
  return set_log_level_helper<SetLogLevelCmd, SetLogLevelRsp>(
      dev_, ::device_api::LOG_LEVELS_TRACE);
}

template class SetLogLevel<
    et_runtime::device_api::devfw_commands::SetMasterLogLevelCmd,
    et_runtime::device_api::devfw_responses::SetMasterLogLevelRsp>;

template class SetLogLevel<
    et_runtime::device_api::devfw_commands::SetWorkerLogLevelCmd,
    et_runtime::device_api::devfw_responses::SetWorkerLogLevelRsp>;

} // namespace et_runtime
