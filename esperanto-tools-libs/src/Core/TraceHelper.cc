//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TraceHelper.h"

#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

#include <memory>
#include <string>

namespace et_runtime {

::device_api::non_privileged::discover_trace_buffer_rsp_t TraceHelper::discover_trace_buffer() {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::DiscoverTraceBufferCmd>(0, 0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id == ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_DISCOVER_TRACE_BUFFER_RSP);
  return response;
}

bool TraceHelper::configure_trace_group_knob(::device_api::non_privileged::trace_groups_e group_id, uint8_t enable) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceGroupKnobCmd>(0, group_id, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_GROUP_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_event_knob(::device_api::non_privileged::trace_events_e event_id, uint8_t enable) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceEventKnobCmd>(0, event_id, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_EVENT_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_buffer_size_knob(uint32_t buffer_size) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceBufferSizeKnobCmd>(0, buffer_size);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_BUFFER_SIZE_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_shire_mask_knob(uint64_t shire_mask) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceShireMaskKnobCmd>(0, shire_mask);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_SHIRE_MASK_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_harts_mask_knob(uint64_t harts_mask) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceHartsMaskKnobCmd>(0, harts_mask);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_HARTS_MASK_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_state_knob(uint8_t enable) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceStateKnobCmd>(0, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_STATE_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::configure_trace_uart_logging_knob(uint8_t enable) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceUartLoggingKnobCmd>(0, enable);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_UART_LOGGING_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::reset_trace_buffers(void) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ResetTraceBuffersCmd>(0, 0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id == ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_RESET_TRACE_BUFFERS_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::prepare_trace_buffers(void) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::PrepareTraceBuffersCmd>(0, 0);

  dev_.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id == ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_PREPARE_TRACE_BUFFERS_RSP);
  return static_cast<bool>(response.status);
}

static bool configure_log_level_knob(Device& dev, ::device_api::non_privileged::log_levels_e log_level) {

  auto log_level_cmd = std::make_shared<device_api::devfw_commands::ConfigureTraceLogLevelKnobCmd>(0, log_level);

  dev.defaultStream().addCommand(log_level_cmd);

  auto response_future = log_level_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_TRACE_LOG_LEVEL_KNOB_RSP);
  return static_cast<bool>(response.status);
}

bool TraceHelper::set_level_critical(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_CRITICAL);
}

bool TraceHelper::set_level_error(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_ERROR);
}

bool TraceHelper::set_level_warning(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_WARNING);
}

bool TraceHelper::set_level_info(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_INFO);
}

bool TraceHelper::set_level_debug(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_DEBUG);
}

bool TraceHelper::set_level_trace(void) {
  return configure_log_level_knob(dev_, ::device_api::non_privileged::LOG_LEVELS_TRACE);
}

void TraceHelper::do_copy(struct ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp,
                          unsigned char* data_buffer, auto hart_counter, uint8_t hart_index,
                          uint8_t total_contiguous_harts, uint8_t shire_index) {
  // Read data from device memory into host buffer for these contiguous harts.
  TraceHelper::dev_.memcpy(
    (void*)(data_buffer + hart_counter * rsp.trace_buffer_size),
    (const void*)(rsp.trace_base +
                  (rsp.trace_buffer_size *
                   ((hart_index - total_contiguous_harts) + (shire_index * sizeof(rsp.harts_mask) * 8))) +
                  ALIGN(sizeof(::device_api::non_privileged::trace_control_t), TRACE_BUFFER_REGION_ALIGNEMNT)),
    total_contiguous_harts * rsp.trace_buffer_size, etrtMemcpyDeviceToHost);
}

uint64_t TraceHelper::extract_device_trace_buffers(struct ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp,
                                                   unsigned char* data_buffer) {
  auto hart_counter = 0;
  uint8_t total_contiguous_harts = 0;
  uint8_t hart_index;
  const uint8_t shire_mask_size = sizeof(rsp.shire_mask) * 8;
  const uint8_t harts_mask_size = sizeof(rsp.harts_mask) * 8;

  /* The following algo finds the contiguous enabled hart and copies the buffer of those harts to reduce DMA
   * Transactions*/
  // Loop for total bits in shire_mask
  for (uint8_t shire_index = 0; shire_index < shire_mask_size; shire_index++) {
    // Check if we need to read data for this shire?
    const bool shire_enabled = (1UL & (rsp.shire_mask >> shire_index)) == 1UL;
    if (shire_enabled) {
      // Loop for total bits in harts_mask
      for (hart_index = 0; hart_index < harts_mask_size; hart_index++) {
        // Finds the total contiguous enabled harts
        const bool hart_enabled = (1UL & (rsp.harts_mask >> hart_index)) == 1UL;
        if (hart_enabled) {
          total_contiguous_harts++;
        } else if (total_contiguous_harts) {
          // Read data from device memory into host buffer for these contiguous harts.
          do_copy(rsp, data_buffer, hart_counter, hart_index, total_contiguous_harts, shire_index);
          hart_counter += total_contiguous_harts;
          total_contiguous_harts = 0;
        }
      }
      // This will take care of last contiguous harts of the shire
      if (total_contiguous_harts) {
        do_copy(rsp, data_buffer, hart_counter, hart_index, total_contiguous_harts, shire_index);
        hart_counter += total_contiguous_harts;
        total_contiguous_harts = 0;
      }
    }
  }
  return (hart_counter * rsp.trace_buffer_size);
}

} // namespace et_runtime
