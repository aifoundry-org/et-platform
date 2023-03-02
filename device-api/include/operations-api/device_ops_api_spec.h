/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

/* WARNING: this file is auto-generated do not edit directly */

#include "esperanto/device-apis/device_apis_build_configuration.h"
#include <stdint.h>

#ifndef ET_DEVICE_OPS_API_SPEC_H
#define ET_DEVICE_OPS_API_SPEC_H

/* High level API specifications */

/*! \def DEVICE_OPS_API_HASH
    \brief 8 LSB bytes MD5 checksum of the device-ops-api spec
*/
#define DEVICE_OPS_API_HASH  0x12174efea0dbdc46ULL

/*! \def DEVICE_OPS_API_MAJOR
    \brief API Major
*/
#define DEVICE_OPS_API_MAJOR ESPERANTO_DEVICE_APIS_VERSION_MAJOR

/*! \def DEVICE_OPS_API_MINOR
    \brief API Minor
*/
#define DEVICE_OPS_API_MINOR ESPERANTO_DEVICE_APIS_VERSION_MINOR

/*! \def DEVICE_OPS_API_PATCH
    \brief API Patch
*/
#define DEVICE_OPS_API_PATCH ESPERANTO_DEVICE_APIS_VERSION_PATCH

/*! \def DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX
    \brief Maximum size of the argument payload supported by kernel launch command
*/
#define DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX 128

/*! \def DEVICE_OPS_DMA_LIST_NODES_MAX
    \brief Maximum number of nodes supported in device-ops DMA list read/write command
*/
#define DEVICE_OPS_DMA_LIST_NODES_MAX             4

/* Device Ops API Enumerations */

typedef uint32_t trace_rt_type_e;

/*! \enum TRACE_RT_TYPE
    \brief 
*/
enum TRACE_RT_TYPE {
  TRACE_RT_TYPE_MM = 1, /**< Master Minion Trace */
  TRACE_RT_TYPE_CM = 2, /**< Compute Minion Trace */
};

typedef uint32_t trace_rt_control_e;

/*! \enum TRACE_RT_CONTROL
    \brief 
*/
enum TRACE_RT_CONTROL {
  TRACE_RT_CONTROL_DISABLE_TRACE = 0, /**< Disable Trace */
  TRACE_RT_CONTROL_ENABLE_TRACE = 1, /**< Enable Trace */
  TRACE_RT_CONTROL_LOG_TO_UART = 2, /**< Redirect Logs to UART */
  TRACE_RT_CONTROL_LOG_TO_TRACE = 0, /**< Redirect Logs to Trace */
  TRACE_RT_CONTROL_RESET_TRACEBUF = 4, /**< Reset Trace buffer, it discards all existing data into Trace buffer. */
};

typedef uint16_t cmd_flags_e;

/*! \enum CMD_FLAGS
    \brief 
*/
enum CMD_FLAGS {
  CMD_FLAGS_BARRIER_DISABLE = 0, /**<  */
  CMD_FLAGS_BARRIER_ENABLE = 1, /**<  */
  CMD_FLAGS_COMPUTE_KERNEL_TRACE_DISABLE = 0, /**<  */
  CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE = 2, /**<  */
  CMD_FLAGS_HOST_MANAGED_UBUF = 0, /**<  */
  CMD_FLAGS_MMFW_TRACEBUF = 4, /**<  */
  CMD_FLAGS_CMFW_TRACEBUF = 8, /**<  */
  CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 = 16, /**<  */
  CMD_FLAGS_KERNEL_LAUNCH_ARGS_EMBEDDED = 32, /**<  */
};

typedef uint32_t dev_ops_api_abort_response_e;

/*! \enum DEV_OPS_API_ABORT_RESPONSE
    \brief 
*/
enum DEV_OPS_API_ABORT_RESPONSE {
  DEV_OPS_API_ABORT_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_ABORT_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_ABORT_RESPONSE_ERROR = 1, /**< Deprecated */
  DEV_OPS_API_ABORT_RESPONSE_INVALID_TAG_ID = 2, /**<  */
};

typedef uint32_t dev_ops_api_cm_reset_response_e;

/*! \enum DEV_OPS_API_CM_RESET_RESPONSE
    \brief 
*/
enum DEV_OPS_API_CM_RESET_RESPONSE {
  DEV_OPS_API_CM_RESET_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_CM_RESET_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_CM_RESET_RESPONSE_INVALID_SHIRE_MASK = 2, /**<  */
  DEV_OPS_API_CM_RESET_RESPONSE_CM_RESET_FAILED = 3, /**<  */
};

typedef uint32_t dev_ops_api_kernel_launch_response_e;

/*! \enum DEV_OPS_API_KERNEL_LAUNCH_RESPONSE
    \brief 
*/
enum DEV_OPS_API_KERNEL_LAUNCH_RESPONSE {
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED = 0, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR = 1, /**< Deprecated */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION = 2, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SHIRES_NOT_READY = 3, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED = 4, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS = 5, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG = 6, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ARGS_PAYLOAD_SIZE = 7, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_MULTICAST_FAILED = 8, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_UNICAST_FAILED = 9, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SP_IFACE_RESET_FAILED = 10, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CW_MINIONS_BOOT_FAILED = 11, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ARGS_INVALID_SHIRE_MASK = 12, /**<  */
  DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_USER_ERROR = 13, /**<  */
};

typedef uint32_t dev_ops_api_kernel_abort_response_e;

/*! \enum DEV_OPS_API_KERNEL_ABORT_RESPONSE
    \brief 
*/
enum DEV_OPS_API_KERNEL_ABORT_RESPONSE {
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR = 1, /**<  */
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID = 2, /**<  */
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_TIMEOUT_HANG = 3, /**<  */
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_HOST_ABORTED = 4, /**<  */
};

typedef uint32_t dev_ops_api_dma_response_e;

/*! \enum DEV_OPS_API_DMA_RESPONSE
    \brief 
*/
enum DEV_OPS_API_DMA_RESPONSE {
  DEV_OPS_API_DMA_RESPONSE_COMPLETE = 0, /**<  */
  DEV_OPS_API_DMA_RESPONSE_UNKNOWN_ERROR = 1, /**< Deprecated */
  DEV_OPS_API_DMA_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE = 2, /**< Deprecated */
  DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED = 3, /**<  */
  DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED = 4, /**<  */
  DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG = 5, /**< Deprecated */
  DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS = 6, /**<  */
  DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE = 7, /**<  */
  DEV_OPS_API_DMA_RESPONSE_CM_IFACE_MULTICAST_FAILED = 8, /**<  */
  DEV_OPS_API_DMA_RESPONSE_DRIVER_DATA_CONFIG_FAILED = 9, /**<  */
  DEV_OPS_API_DMA_RESPONSE_DRIVER_LINK_CONFIG_FAILED = 10, /**<  */
  DEV_OPS_API_DMA_RESPONSE_DRIVER_CHAN_START_FAILED = 11, /**<  */
  DEV_OPS_API_DMA_RESPONSE_DRIVER_ABORT_FAILED = 12, /**<  */
};

typedef uint32_t dev_ops_api_echo_response_e;

/*! \enum DEV_OPS_API_ECHO_RESPONSE
    \brief 
*/
enum DEV_OPS_API_ECHO_RESPONSE {
  DEV_OPS_API_ECHO_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_ECHO_RESPONSE_HOST_ABORTED = 1, /**<  */
};

typedef uint32_t dev_ops_api_fw_version_response_e;

/*! \enum DEV_OPS_API_FW_VERSION_RESPONSE
    \brief 
*/
enum DEV_OPS_API_FW_VERSION_RESPONSE {
  DEV_OPS_API_FW_VERSION_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_FW_VERSION_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_FW_VERSION_RESPONSE_BAD_FW_TYPE = 2, /**<  */
  DEV_OPS_API_FW_VERSION_RESPONSE_NOT_AVAILABLE = 3, /**<  */
  DEV_OPS_API_FW_VERSION_RESPONSE_HOST_ABORTED = 4, /**<  */
};

typedef uint32_t dev_ops_api_compatibility_response_e;

/*! \enum DEV_OPS_API_COMPATIBILITY_RESPONSE
    \brief 
*/
enum DEV_OPS_API_COMPATIBILITY_RESPONSE {
  DEV_OPS_API_COMPATIBILITY_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_API_COMPATIBILITY_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_API_COMPATIBILITY_RESPONSE_HOST_ABORTED = 2, /**<  */
};

typedef uint32_t dev_ops_api_error_type_e;

/*! \enum DEV_OPS_API_ERROR_TYPE
    \brief 
*/
enum DEV_OPS_API_ERROR_TYPE {
  DEV_OPS_API_ERROR_TYPE_UNSUPPORTED_COMMAND = 0, /**<  */
  DEV_OPS_API_ERROR_TYPE_CM_SMODE_RT_EXCEPTION = 1, /**<  */
  DEV_OPS_API_ERROR_TYPE_CM_SMODE_RT_HANG = 2, /**<  */
};

typedef uint8_t dev_ops_fw_type_e;

/*! \enum DEV_OPS_FW_TYPE
    \brief 
*/
enum DEV_OPS_FW_TYPE {
  DEV_OPS_FW_TYPE_MASTER_MINION_FW = 0, /**<  */
  DEV_OPS_FW_TYPE_MACHINE_MINION_FW = 1, /**<  */
  DEV_OPS_FW_TYPE_WORKER_MINION_FW = 2, /**<  */
};

typedef uint32_t dev_ops_trace_rt_config_response_e;

/*! \enum DEV_OPS_TRACE_RT_CONFIG_RESPONSE
    \brief 
*/
enum DEV_OPS_TRACE_RT_CONFIG_RESPONSE {
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_SHIRE_MASK = 2, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_THREAD_MASK = 3, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_EVENT_MASK = 4, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_FILTER_MASK = 5, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_RT_CONFIG_ERROR = 6, /**< Deprecated */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_HOST_ABORTED = 7, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_CM_TRACE_CONFIG_FAILED = 8, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_MM_TRACE_CONFIG_FAILED = 9, /**<  */
  DEV_OPS_TRACE_RT_CONFIG_RESPONSE_INVALID_TRACE_CONFIG_INFO = 10, /**<  */
};

typedef uint32_t dev_ops_trace_rt_control_response_e;

/*! \enum DEV_OPS_TRACE_RT_CONTROL_RESPONSE
    \brief 
*/
enum DEV_OPS_TRACE_RT_CONTROL_RESPONSE {
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS = 0, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_UNEXPECTED_ERROR = 1, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE = 2, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_CONTROL_MASK = 3, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_RT_CTRL_ERROR = 4, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_MM_RT_CTRL_ERROR = 5, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_HOST_ABORTED = 6, /**<  */
  DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_IFACE_MULTICAST_FAILED = 7, /**<  */
};

/*! \enum device_ops_api_msg_e
    \brief Currently MM Firmware needs both cmd/rsp IDs. So this enum is retained for OPS node
    Enumeration of all the RPC messages that the Device Ops API
    send/receive.
*/
enum device_ops_api_msg_e {
    DEV_OPS_API_MID_NONE = 512,
    DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD = 513, /**< < Request the version of the device_ops_api supported by the target */
    DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP = 514, /**< < Return the version implemented by the target */
    DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD = 515, /**< < Request the version of the device firmware, advertise the one we support */
    DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP = 516, /**< < Return the device fw version for the requested type */
    DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD = 517, /**< < Echo command */
    DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP = 518, /**< < Echo response */
    DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD = 519, /**< < Command to abort a currently pipelined command in the device */
    DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP = 520, /**< < Response to an abort request */
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD = 521, /**< < Launch a kernel on the target */
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP = 522, /**< < Response and result of a kernel launch on the device */
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD = 523, /**< < Command to abort a currently running kernel on the device */
    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP = 524, /**< < Response to an abort request */
    DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD = 525, /**< < Single list Command to perform multiple DMA reads from device memory */
    DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP = 526, /**< < DMA readlist command response */
    DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD = 527, /**< < Single list Command to perform multiple DMA writes on device memory */
    DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP = 528, /**< < DMA writelist command response */
    DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD = 529, /**< < Configure the trace configuration */
    DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP = 530, /**< < Trace configure command reply */
    DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD = 531, /**< < Configure options to start, stop, OR fetch trace logs. */
    DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP = 532, /**< < Trace runtime control command reply */
    DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_CMD = 533, /**< < CM Reset command */
    DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_RSP = 534, /**< < CM Reset response */
    DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR = 535, /**< < Generic error reported by the device */
    DEV_OPS_API_MID_DEVICE_OPS_TRACE_BUFFER_FULL_EVENT = 536, /**< < Event triggered when trace buffer has reached threshold or is full */
    DEV_OPS_API_MID_LAST  = 1023
};

#endif /* ET_DEVICE_OPS_API_SPEC_H */