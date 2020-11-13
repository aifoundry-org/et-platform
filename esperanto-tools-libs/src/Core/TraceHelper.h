//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_TRACE_HELPERS_H
#define ET_RUNTIME_TRACE_HELPERS_H

#include <cstdint>
#include <esperanto/device-api/device_api_cxx_non_privileged.h>

namespace et_runtime {

class Device;

// Forward declarations of commands and responses
namespace device_api {
namespace devfw_commands {
class DiscoverTraceBufferCmd;
class ConfigureTraceLogLevelKnobCmd;
class ConfigureTraceGroupKnobCmd;
class ConfigureTraceEventKnobCmd;
class ConfigureTraceBufferSizeKnobCmd;
class ConfigureTraceShireMaskKnobCmd;
class ConfigureTraceHartsMaskKnobCmd;
class ConfigureTraceStateKnobCmd;
class ConfigureTraceUartLoggingKnobCmd;
class ResetTraceBuffersCmd;
class PrepareTraceBuffersCmd;
} // namespace devfw_commands

namespace devfw_responses {
class DiscoverTraceBufferRsp;
class ConfigureTraceLogLevelKnobRsp;
class ConfigureTraceGroupKnobRsp;
class ConfigureTraceEventKnobRsp;
class ConfigureTraceBufferSizeKnobRsp;
class ConfigureTraceShireMaskKnobRsp;
class ConfigureTraceHartsMaskKnobRsp;
class ConfigureTraceStateKnobRsp;
class ConfigureTraceUartLoggingKnobRsp;
class ResetTraceBuffersRsp;
class PrepareTraceBuffersRsp;
} // namespace devfw_responses
} // namespace device_api

/// @class TraceHelper TraceHelper.h
///
/// @brief Class holding helpers to configure trace subsystem, using the
/// Device API and help us interact with the device
///
class TraceHelper {
public:
  /// @brierf TraceHelper constructor
  ///
  /// @param[in] dev Reference to the associate device
  TraceHelper(Device& dev)
    : dev_(dev){};

  ///
  /// @brief Sends discover command to device and to query necessary information
  /// about the trace subsystem, which includes trace base address and buffer
  /// size
  ///
  /// @returns discover_trace_buffer_rsp_t
  ::device_api::non_privileged::discover_trace_buffer_rsp_t discover_trace_buffer();

  ///
  /// @brief Allows to enable or disable a particular Group of events
  ///
  /// @param[in] group_id : Group id to enable/disable
  /// @param[in] enable  : State of the group to set
  /// @returns true, false
  bool configure_trace_group_knob(::device_api::non_privileged::trace_groups_e group_id, uint8_t enable);

  ///
  /// @brief Allows to enable or disable a particular Event
  ///
  /// @param[in] event_id : Event id to enable/disable
  /// @param[in] enable  : State of the group to set
  /// @returns true, false
  bool configure_trace_event_knob(::device_api::non_privileged::trace_events_e event_id, uint8_t enable);

  ///
  /// @brief Allows to configure trace buffer size
  ///
  /// @param[in] buffer_size : New buffer size to configure
  /// @returns true, false
  bool configure_trace_buffer_size_knob(uint32_t buffer_size);

  ///
  /// @brief Allows to configure trace shire mask
  ///
  /// @param[in] shire_mask : New trace shire mask to configure
  /// @returns true, false
  bool configure_trace_shire_mask_knob(uint64_t shire_mask);

  ///
  /// @brief Allows to configure trace harts mask
  ///
  /// @param[in] harts_mask : New trace harts mask to configure
  /// @returns true, false
  bool configure_trace_harts_mask_knob(uint64_t harts_mask);

  ///
  /// @brief Allows to enable or disable complete device side tracing
  ///
  /// @param[in] enable : Intended state of the trace component
  /// @returns true, false
  bool configure_trace_state_knob(uint8_t enable);

  ///
  /// @brief Allows to enable/disable logging of trace events on Uart interface
  ///
  /// @param[in] enable : Intended Uart logging state
  /// @returns true, false
  bool configure_trace_uart_logging_knob(uint8_t enable);

  ///
  /// @brief Allows to reset trace buffers for next run
  ///
  /// @returns true, false
  bool reset_trace_buffers(void);

  ///
  /// @brief Prepares trace buffer for consumption
  ///
  /// @returns true, false
  bool prepare_trace_buffers(void);

  ///
  /// @brief Configures event log level to critical
  ///
  /// @returns true, false
  bool set_level_critical(void);

  ///
  /// @brief Configures event log level to error
  ///
  /// @returns true, false
  bool set_level_error(void);

  ///
  /// @brief Configures event log level to warning
  ///
  /// @returns true, false
  bool set_level_warning(void);

  ///
  /// @brief Configures event log level to info
  ///
  /// @returns true, false
  bool set_level_info(void);

  ///
  /// @brief Configures event log level to debug
  ///
  /// @returns true, false
  bool set_level_debug(void);

  ///
  /// @brief Configures event log level to trace
  ///
  /// @returns true, false
  bool set_level_trace(void);

  ///
  /// @brief Copy the buffer data from shires/harts to dest_ptr
  ///
  /// @returns true, uint64_t
  uint64_t extract_device_trace_buffers(struct ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp,
                                        unsigned char* dest_ptr);

private:
  Device& dev_; ///< Device object, this class interacts with
  void do_copy(struct ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp, unsigned char* data_buffer,
               auto hart_counter, uint8_t hart_index, uint8_t total_contiguous_harts, uint8_t shire_index);
};

} // namespace et_runtime

#endif // ET_RUNTIME_TRACE_HELPERS_H
