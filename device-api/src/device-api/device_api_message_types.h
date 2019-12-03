/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_API_MESSAGE_TYPES_H
#define ET_DEVICE_API_MESSAGE_TYPES_H

#include <stdint.h>


// WARNING: The following data-structures are part of the device-api data exchanged between
// host and device. The requirement is that fields are naturally aligned. To easily enforce this
// make sure that member fields are ordered by decreasing size.

#ifdef __cplusplus
namespace device_api {
#endif

typedef uint64_t mbox_message_id_t;

/// @brief common header that is expected to be at the beginning of each device-api message
/// and it is going to be used to identify the type of hte message
struct common_header_t {
    mbox_message_id_t message_id;  /// Type of the message
};

/// @brief Common header for all the command messages coming from the host
struct command_header_t {
    mbox_message_id_t message_id;  /// Type of the message
    uint64_t command_id; /// Unique ID issued by the host to differentiate the different commands
                         /// sent to the device
    uint64_t host_timestamp; /// Timestamp on the host when the command as created. Used for performance
                             /// tracking
    uint64_t device_timestamp_mtime;  /// mtime arrival timestamp of the message. Used for performance tracking
    uint32_t stream_id; /// Unique ID idetntifying the host stream that issued the command, necessary
                        /// for routing the response back to the appropriate host stream
};

/// @brief Common header for all reply messages from the device to the host
struct response_header_t {
    mbox_message_id_t message_id; /// Type of the message
    struct command_header_t  command_info; /// Copy of the information of the original command
                                           /// this response corresponds to
    uint64_t device_timestamp_mtime;  /// mtime departute timestamp of the message. Used for performance tracking
};


/// @brief Common header for all event messages taht the device can send to the host
struct event_header_t {
    mbox_message_id_t message_id; /// Type of the message
    uint64_t device_timestamp_mtime; /// Timestamp on the device, when the response is enqueued in the mailbox
};

static inline mbox_message_id_t get_device_api_message_id(const void* msg) {
    const struct common_header_t* header = (const struct common_header_t*) msg;
    return header->message_id;
}

#ifdef __cplusplus
} // namespace device_api
#endif

#endif // ET_DEVICE_API_MESSAGE_TYPES_H
