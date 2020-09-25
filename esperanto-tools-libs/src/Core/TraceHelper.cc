//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/TraceHelper.h"
#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

#include <memory>
#include <string>

namespace et_runtime {

::device_api::non_privileged::discover_trace_buffer_rsp_t
TraceHelper::discover_trace_buffer() {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::DiscoverTraceBufferCmd>(0,
                                                                           0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_DISCOVER_TRACE_BUFFER_RSP);
  return response;
}

bool TraceHelper::configure_trace_group_knob(
    ::device_api::non_privileged::trace_groups_e group_id, uint8_t enable) {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::ConfigureTraceGroupKnobCmd>(
          0, group_id, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::
             MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_GROUP_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_event_knob(
    ::device_api::non_privileged::trace_events_e event_id, uint8_t enable) {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::ConfigureTraceEventKnobCmd>(
          0, event_id, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::
             MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_EVENT_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_buffer_size_knob(uint32_t buffer_size) {

  auto log_level_cmd = std::make_shared<
      device_api::devfw_commands::ConfigureTraceBufferSizeKnobCmd>(0,
                                                                   buffer_size);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::
          MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_BUFFER_SIZE_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_shire_mask_knob(uint64_t shire_mask) {

  auto log_level_cmd = std::make_shared<
      device_api::devfw_commands::ConfigureTraceShireMaskKnobCmd>(0,
                                                                   shire_mask);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::
          MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_SHIRE_MASK_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_harts_mask_knob(uint64_t harts_mask) {

  auto log_level_cmd = std::make_shared<
      device_api::devfw_commands::ConfigureTraceHartsMaskKnobCmd>(0,
                                                                   harts_mask);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::
          MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_HARTS_MASK_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_state_knob(uint8_t enable) {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::ConfigureTraceStateKnobCmd>(
          0, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::
             MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_STATE_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_uart_logging_knob(uint8_t enable) {

  auto log_level_cmd = std::make_shared<
      device_api::devfw_commands::ConfigureTraceUartLoggingKnobCmd>(0, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::
          MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_UART_LOGGING_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::reset_trace_buffers(void) {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::ResetTraceBuffersCmd>(0, 0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_RESET_TRACE_BUFFERS_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::prepare_trace_buffers(void) {

  auto log_level_cmd =
      std::make_shared<device_api::devfw_commands::PrepareTraceBuffersCmd>(0,
                                                                           0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(
      response.response_info.message_id ==
      ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_PREPARE_TRACE_BUFFERS_RSP);
  return static_cast<bool>(response.status);
}

static bool
configure_log_level_knob(Device &dev,
                         ::device_api::non_privileged::log_levels_e log_level) {

  auto log_level_cmd = std::make_shared<
      device_api::devfw_commands::ConfigureTraceLogLevelKnobCmd>(0, log_level);

  dev.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::
             MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_LOG_LEVEL_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::set_level_critical(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_CRITICAL);
}

bool TraceHelper::set_level_error(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_ERROR);
}

bool TraceHelper::set_level_warning(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_WARNING);
}

bool TraceHelper::set_level_info(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_INFO);
}

bool TraceHelper::set_level_debug(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_DEBUG);
}

bool TraceHelper::set_level_trace(void) {
  return configure_log_level_knob(
      dev_, ::device_api::non_privileged::LOG_LEVELS_TRACE);
}

} // namespace et_runtime
