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

#ifndef ET_DEVICE_OPS_API_SPEC_H
#define ET_DEVICE_OPS_API_SPEC_H

/// High level API specifications

/// 8 LSB bytes MD5 checksum of the device-ops-api spec
#define DEVICE_OPS_API_HASH  0x90863a829d54e00eULL

/// API Major
#define DEVICE_OPS_API_MAJOR 0

/// API Minor
#define DEVICE_OPS_API_MINOR 1

/// API Patch
#define DEVICE_OPS_API_PATCH 0

//ToDo: Remove commented content
/// Device Ops API enum storage type
//typedef uint16_t device_ops_api_tag_id_t;
/// Device Ops API enum storage type
//typedef uint8_t device_ops_api_cmd_id_t;

/// @enum Enumeration of all the RPC messages that the Device Ops API
/// send/receive
enum device_ops_api_msg_e {
    DEV_OPS_API_MID_NONE = 0,
    DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD = 1, ///< Request the version of the device_ops_api supported by the target, advertise the one we support
    DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP = 2, ///< Return the version implemented by the target
    DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD = 3, ///< Request the version of the device firmware, advertise the one we support
    DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP = 4, ///< Return the device fw version for the requested type
    DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD = 5, ///< Echo command
    DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP = 6, ///< Echo response
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD = 7, ///< Launch a kernel on the target
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP = 8, ///< Response and result of a kernel launch on the device
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD = 9, ///< Command to abort a currently running kernel on the device
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP = 10, ///< Response to an abort request
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD = 11, ///< Query the state if a currently running kernel
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_RSP = 12, ///< Kernel state reply
    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD = 13, ///< Command to read data from device memory
    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP = 14, ///< Data read command response
    DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD = 15, ///< Command to write data to device memory
    DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP = 16, ///< Data write command response
    DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR = 17, ///< Generic error reported by the device
    DEV_OPS_API_MID_LAST  = 18
};

#endif // ET_DEVICE_OPS_API_SPEC_H