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

#ifndef ET_DEVICE_OPS_API_RPC_TYPES_H
#define ET_DEVICE_OPS_API_RPC_TYPES_H

#include "device_apis_message_types.h"

// Device Ops API Enumerations
typedef uint32_t dev_ops_api_kernel_launch_status_e;

/// @brief
enum DEV_OPS_API_KERNEL_LAUNCH_STATUS {
  DEV_OPS_API_KERNEL_LAUNCH_STATUS_OK = 0,
  DEV_OPS_API_KERNEL_LAUNCH_STATUS_SHIRES_NOT_READY = 1,
  DEV_OPS_API_KERNEL_LAUNCH_STATUS_ERROR = 2,
  DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_OK = 3,
  DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_ERROR = 4,

};

typedef uint32_t dev_ops_api_kernel_abort_response_e;

/// @brief
enum DEV_OPS_API_KERNEL_ABORT_RESPONSE {
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_NONE = 0,
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_OK = 1,
  DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR = 2,

};

typedef uint32_t dev_ops_api_kernel_state_e;

/// @brief
enum DEV_OPS_API_KERNEL_STATE {
  DEV_OPS_API_KERNEL_STATE_UNUSED = 0,
  DEV_OPS_API_KERNEL_STATE_LAUNCHED = 1,
  DEV_OPS_API_KERNEL_STATE_RUNNING = 2,
  DEV_OPS_API_KERNEL_STATE_ABORTED = 3,
  DEV_OPS_API_KERNEL_STATE_ERROR = 4,
  DEV_OPS_API_KERNEL_STATE_COMPLETE = 5,

};

typedef uint32_t etsoc_dma_state_e;

/// @brief State of a DMA
enum ETSOC_DMA_STATE {
  ETSOC_DMA_STATE_IDLE = 0,
  ETSOC_DMA_STATE_ACTIVE = 1,
  ETSOC_DMA_STATE_DONE = 2,
  ETSOC_DMA_STATE_ABORTING = 3,
  ETSOC_DMA_STATE_ABORTED = 4,

};

typedef uint32_t dev_ops_api_error_type_e;

/// @brief
enum DEV_OPS_API_ERROR_TYPE {
  DEV_OPS_API_ERROR_TYPE_NONE = 0,
  DEV_OPS_API_ERROR_TYPE_ASYNC_ERROR_0 = 1,

};

typedef uint8_t dev_ops_fw_type_e;

/// @brief
enum DEV_OPS_FW_TYPE {
  DEV_OPS_FW_TYPE_MASTER_MINION_FW = 0,
  DEV_OPS_FW_TYPE_MACHINE_MINION_FW = 1,
  DEV_OPS_FW_TYPE_WORKER_MINION_FW = 2,

};



// Device Ops API structs
// Structs that are being used in other tests



// The real Device Ops API RPC messages that we exchange

/// @brief Request the version of the device_ops_api supported by the target, advertise the one we support
struct check_device_ops_api_compatibility_cmd_t {
  struct cmd_header_t command_info;
  uint8_t  major; /// Sem Version Major
  uint8_t  minor; /// Sem Version Minor
  uint8_t  patch; /// Sem Version Patch
  uint8_t  pad; /// pad
};

/// @brief Return the version implemented by the target
struct device_ops_api_compatibility_rsp_t {
  struct rsp_header_t response_info;
  uint8_t  major; /// Sem Version Major
  uint8_t  minor; /// Sem Version Minor
  uint8_t  patch; /// Sem Version Patch
  uint8_t  pad; /// pad
};

/// @brief Request the version of the device firmware, advertise the one we support
struct device_ops_device_fw_version_cmd_t {
  struct cmd_header_t command_info;
  uint8_t  firmware_type; /// Identify device firmware type
};

/// @brief Return the device fw version for the requested type
struct device_ops_fw_version_rsp_t {
  struct rsp_header_t response_info;
  uint8_t  major; /// Sem Version Major
  uint8_t  minor; /// Sem Version Minor
  uint8_t  patch; /// Sem Version Patch
  dev_ops_fw_type_e  type; /// Device firmware type
};

/// @brief Echo command
struct device_ops_echo_cmd_t {
  struct cmd_header_t command_info;
  int32_t  echo_payload; /// Echo payload field
};

/// @brief Echo response
struct device_ops_echo_rsp_t {
  struct rsp_header_t response_info;
  int32_t  echo_payload; /// Echo payload field
};

/// @brief Launch a kernel on the target
struct device_ops_kernel_launch_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  code_start_address; /// Code start address
  uint64_t  pointer_to_args; /// Pointer to kernel arguments
};

/// @brief Response and result of a kernel launch on the device
struct device_ops_kernel_launch_rsp_t {
  struct rsp_header_t response_info;
  uint64_t  cmd_wait_time; /// Time transpired between command arrival and dispatch
  uint64_t  cmd_execution_time; /// Time transpired between command dispatch and command completion
  dev_ops_api_kernel_launch_status_e  status; /// Kernel Launch status
};

/// @brief Command to abort a currently running kernel on the device
struct device_ops_kernel_abort_cmd_t {
  struct cmd_header_t command_info;
  uint8_t  kernel_id; /// Kernel Identifier
};

/// @brief Response to an abort request
struct device_ops_kernel_abort_rsp_t {
  struct rsp_header_t response_info;
  dev_ops_api_kernel_abort_response_e  status; /// Kernel Abort status
  uint8_t  kernel_id; /// Kernel Identifier
};

/// @brief Query the state if a currently running kernel
struct device_ops_kernel_state_cmd_t {
  struct cmd_header_t command_info;
  uint8_t  kernel_id; ///
};

/// @brief Kernel state reply
struct device_ops_kernel_state_rsp_t {
  struct rsp_header_t response_info;
  dev_ops_api_kernel_state_e  status; /// Kernel Abort status
};

/// @brief Command to read data from device memory
struct device_ops_data_read_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  dst_host_virt_addr; /// Host Virtual Address in userspace
  uint64_t  dst_host_phy_addr; /// Host Physical Address in kernelspace
  uint64_t  src_device_phy_addr; /// Device Address
  uint32_t  size; /// Size
};

/// @brief Data read command response
struct device_ops_data_read_rsp_t {
  struct rsp_header_t response_info;
  uint64_t  cmd_wait_time; /// Time transpired between command arrival and dispatch
  uint64_t  cmd_execution_time; /// Time transpired between command dispatch and command completion
  etsoc_dma_state_e  status; /// Status of the DMA operation
};

/// @brief Command to write data to device memory
struct device_ops_data_write_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  src_host_virt_addr; /// Host Virtual Address in userspace
  uint64_t  src_host_phy_addr; /// Host Physical Address in kernelspace
  uint64_t  dst_device_phy_addr; /// Device Address
  uint32_t  size; /// Size
};

/// @brief Data write command response
struct device_ops_data_write_rsp_t {
  struct rsp_header_t response_info;
  uint64_t  cmd_wait_time; /// Time transpired between command arrival and dispatch
  uint64_t  cmd_execution_time; /// Time transpired between command dispatch and command completion
  etsoc_dma_state_e  status; /// Status of the DMA operation
};

/// @brief Generic error reported by the device
struct device_ops_device_fw_error_t {
  struct evt_header_t event_info;
  dev_ops_api_error_type_e  error_type; ///
};


#endif // ET_DEVICE_OPS_API_RPC_TYPES_H
