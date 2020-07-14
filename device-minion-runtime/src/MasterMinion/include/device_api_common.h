#ifndef DEVICE_API_COMMON_H
#define DEVICE_API_COMMON_H

#include <stdint.h>
#include <esperanto/device-api/device_api_message_types.h>

/// \brief common preparation steps for a DeviceAPI reply back to the host
/// \param[in] cmd: Pointer to the command this reply corresponds to. The command header
///        will be copied in the response body for the runtime to identify which is the original
///        command the reply corresponds to
void prepare_device_api_reply(const struct command_header_t* const cmd,
                              struct response_header_t* const rsp);

#endif
