/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_APIS_MESSAGE_TYPES_H
#define ET_DEVICE_APIS_MESSAGE_TYPES_H

// WARNING: The following data-structures are part of the device-api data exchanged between
// host and device. The requirement is that fields are naturally aligned. To easily enforce this
// make sure that member fields are ordered by decreasing size.
typedef uint16_t tag_id_t;
typedef uint16_t msg_id_t;

/// @brief Common header, common to command, response, and events
struct cmn_header_t {
    uint16_t size; ///< size of payload that follows the message header
    tag_id_t tag_id; ///< unique ID to correlate commands/responses across Host-> Device
    msg_id_t msg_id; ///< unique ID to differentiate commands/responses/events generated from host
};

/// @brief Command header for all commands host to device
struct cmd_header_t {
    struct cmn_header_t cmd_hdr; ///< Command header
    uint16_t flags;   ///< flags bitmask, (1<<0) = barrier, (1<<1) = enable time stamps.
};

/// @brief Response header for all command responses from device to host
struct rsp_header_t {
    struct cmn_header_t rsp_hdr; ///< Response header
};

/// @brief Event header for all events from device to host
struct evt_header_t {
    struct cmn_header_t evt_hdr; ///< Event header, tag_id is dont care for events.
};

#endif // ET_DEVICE_APIS_MESSAGE_TYPES_H
