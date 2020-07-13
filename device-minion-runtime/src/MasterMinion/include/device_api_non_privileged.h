#ifndef DEVICE_API_NON_PRIVILEGED_H
#define DEVICE_API_NON_PRIVILEGED_H

#include "kernel_params.h"
#include "kernel_info.h"
#include "log_levels.h"
#include "mailbox_id.h"

#include <stdint.h>
#include <esperanto/device-api/device_api.h>
#include <esperanto/device-api/device_api_message_types.h>

/// \brief common preparation steps for a DeviceAPI reply back to the host
/// \param[in] cmd: Pointer to the command this reply corresponds to. The command header
///        will be copied in the response body for the runtime to identify which is the original
///        command the reply corresponds to
void prepare_device_api_reply(const struct command_header_t *const cmd,
                              struct response_header_t *const rsp);

/// \brief Convert the log-level enum from the one defined in the DeviceAPI to the FW internal type
log_level_t devapi_loglevel_to_fw(const enum LOG_LEVELS log_level);

/// \brief Handle a DeviceAPI command from the host
/// \param[in] message_id: Type of the command we have received
/// \param[in] buffer: Pointer to the data. The buffer is not made const in purpose
///             to allow us to modify the contents of the message upon arrive and record
///             necessary additional information, like timestamps. We also expect that the
///             buffer has the expected size for the type of the message held
void handle_device_api_non_privileged_message_from_host(const mbox_message_id_t* message_id,
                                                        uint8_t* buffer);

#endif
