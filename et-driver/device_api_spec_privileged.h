/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

// WARNING: this file is auto-generated do not edit directly

#ifndef ET_DEVICE_API_SPEC_PRIVILEGED_H
#define ET_DEVICE_API_SPEC_PRIVILEGED_H

/// High level API specifications

/// 8 LSB bytes MD5 checksum of the device-api spec
#define DEVICE_API_PRIVILEGED_HASH  0x9fdc478830f3e466ULL

/// API Major
#define DEVICE_API_PRIVILEGED_MAJOR 0

/// API Minor
#define DEVICE_API_PRIVILEGED_MINOR 1

/// API Patch
#define DEVICE_API_PRIVILEGED_PATCH 0

/// Device API Privileged-segment enum storate type
typedef uint64_t device_api_privileged_msg_t;

/// @enum Enumeration of all the RPC messages that the Privileged segment
/// of the Device API and send/receive
enum device_api_privileged_msg_e {
    MBOX_DEVAPI_PRIVILEGED_MID_NONE = 0,
    MBOX_DEVAPI_PRIVILEGED_MID_DEVICE_API_VERSION_CMD = 1, ///< Request the version of the device_api privileged-level supported by the target, advertise the one we support
    MBOX_DEVAPI_PRIVILEGED_MID_DEVICE_API_VERSION_RSP = 2, ///< Return the version implemented by the target
    MBOX_DEVAPI_PRIVILEGED_MID_REFLECT_CMD = 3, ///< Execute the reflect test cmd
    MBOX_DEVAPI_PRIVILEGED_MID_REFLECT_RSP = 4, ///< Response to reflect test
    MBOX_DEVAPI_PRIVILEGED_MID_DMA_RUN_TO_DONE_CMD = 5, ///< Execute a DMA on channel until it completes
    MBOX_DEVAPI_PRIVILEGED_MID_DMA_RUN_TO_DONE_RSP = 6, ///< Response marking the commpletion of a DMA
    MBOX_DEVAPI_PRIVILEGED_MID_LAST  = 999
};

#endif // ET_DEVICE_API_SPEC_PRIVILEGED_H