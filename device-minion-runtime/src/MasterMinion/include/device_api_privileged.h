#ifndef DEVICE_API_H
#define DEVICE_API_H

#include "device_api_common.h"
#include "log_levels.h"
#include "mailbox_id.h"

#include <stdint.h>
#include <esperanto/device-api/device_api.h>
#include <esperanto/device-api/device_api_message_types.h>

/// \brief Handle a DeviceAPI command from the host
/// \param[in] message_id: Type of the command we have received
/// \param[in] buffer: Pointer to the data. The buffer is not made const in purpose
///             to allow us to modify the contents of the message upon arrive and record
///             necessary additional information, like timestamps. We also expect that the
///             buffer has the expected size for the type of the message held
void handle_device_api_privileged_message_from_host(const mbox_message_id_t* message_id,
                                                    uint8_t* buffer);


#endif
