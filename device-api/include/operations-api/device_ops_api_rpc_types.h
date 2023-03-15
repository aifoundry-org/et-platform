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

#ifndef ET_DEVICE_OPS_API_RPC_TYPES_H
#define ET_DEVICE_OPS_API_RPC_TYPES_H

#include "esperanto/device-apis/device_apis_message_types.h"
#include <stdint.h>

/* Device Ops API structs */
/* Structs that are being used in other tests */

/*! \struct dma_read_node
    \brief Node containing one DMA read transfer information
*/
struct dma_read_node {
  uint64_t  dst_host_virt_addr; /**< Host Virtual Address */
  uint64_t  dst_host_phy_addr; /**< Host Physical Address */
  uint64_t  src_device_phy_addr; /**< Device Address */
  uint32_t  size; /**< Size */
  uint8_t  pad[4]; /**< Padding for alignment */
  
} __attribute__((packed));

/*! \struct dma_write_node
    \brief Node containing one DMA write transfer information
*/
struct dma_write_node {
  uint64_t  src_host_virt_addr; /**< Host Virtual Address */
  uint64_t  src_host_phy_addr; /**< Host Physical Address */
  uint64_t  dst_device_phy_addr; /**< Device Address */
  uint32_t  size; /**< Size */
  uint8_t  pad[4]; /**< Padding for alignment */
  
} __attribute__((packed));

/*! \struct p2pdma_node
    \brief Node containing one P2P DMA (read/write) transfer information
*/
struct p2pdma_node {
  uint64_t  dst_device_phy_addr; /**< Physical address in src_device Host Managed DRAM range */
  uint64_t  src_device_phy_addr; /**< Physical address in dst_device Host Managed DRAM range */
  uint64_t  pci_p2pmem_bus_addr; /**< Translated bus address of peer partner device physical address */
  uint32_t  size; /**< Size */
  uint16_t  peer_devnum; /**< The devnum of the peer partner device in P2P DMA */
  uint16_t  pad; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct kernel_rsp_error_ptr_t
    \brief This contains U-mode exception buffer pointer and U-mode trace buffer pointer
*/
struct kernel_rsp_error_ptr_t {
  uint64_t  umode_exception_buffer_ptr; /**< Pointer to the U-mode exception buffer */
  uint64_t  umode_trace_buffer_ptr; /**< Pointer to the U-mode trace buffer */
  uint64_t  cm_shire_mask; /**< Bit mask showing in which CM shire exception OR hang occurred */
  
} __attribute__((packed));



/* The real Device Ops API RPC messages that we exchange */

/*! \struct check_device_ops_api_compatibility_cmd_t
    \brief Request the version of the device_ops_api supported by the target
*/
struct check_device_ops_api_compatibility_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_api_compatibility_rsp_t
    \brief Return the version implemented by the target
*/
struct device_ops_api_compatibility_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_api_compatibility_response_e  status; /**< Compatibility status */
  uint16_t  major; /**< Sem Version Major */
  uint16_t  minor; /**< Sem Version Minor */
  uint16_t  patch; /**< Sem Version Patch */
  uint8_t  pad[6]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_device_fw_version_cmd_t
    \brief Request the version of the device firmware, advertise the one we support
*/
struct device_ops_device_fw_version_cmd_t {
  struct cmd_header_t command_info;
  dev_ops_fw_type_e  firmware_type; /**< Identify device firmware type */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_fw_version_rsp_t
    \brief Return the device fw version for the requested type
*/
struct device_ops_fw_version_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_api_fw_version_response_e  status; /**< Firmware version status */
  uint16_t  major; /**< Sem Version Major */
  uint16_t  minor; /**< Sem Version Minor */
  uint16_t  patch; /**< Sem Version Patch */
  dev_ops_fw_type_e  type; /**< Device firmware type */
  uint8_t  pad[5]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_echo_cmd_t
    \brief Echo command
*/
struct device_ops_echo_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_echo_rsp_t
    \brief Echo response
*/
struct device_ops_echo_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Report of the time stamp (in uS) when the command got popped out of the Submission Queue. */
  dev_ops_api_echo_response_e  status; /**< Echo command status */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_abort_cmd_t
    \brief Command to abort a currently pipelined command in the device
*/
struct device_ops_abort_cmd_t {
  struct cmd_header_t command_info;
  uint16_t  tag_id; /**< Tag ID of the pipelined command the we want to abort */
  uint8_t  pad[6]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_abort_rsp_t
    \brief Response to an abort request
*/
struct device_ops_abort_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_api_abort_response_e  status; /**< Abort status */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_kernel_launch_cmd_t
    \brief Launch a kernel on the target
*/
struct device_ops_kernel_launch_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  code_start_address; /**< Starting address of the location of the Compute Kernel code */
  uint64_t  pointer_to_args; /**< Pointer to kernel arguments */
  uint64_t  exception_buffer; /**< Pointer to the exception buffer */
  uint64_t  shire_mask; /**< BitMask indicating Compute Shires that is used to execute a given Kernel */
  uint64_t  argument_payload[]; /**< These are optional payload that can go up to 128 bytes */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_kernel_launch_rsp_t
    \brief Response and result of a kernel launch on the device
*/
struct device_ops_kernel_launch_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Timestamp (in cycles) at which the command was dispatched */
  uint64_t  device_cmd_execute_dur; /**< Time transpired between command dispatch and command completion */
  uint64_t  device_cmd_wait_dur; /**< Time transpired between command arrival and dispatch */
  dev_ops_api_kernel_launch_response_e  status; /**< Kernel Launch status */
  uint32_t  pad; /**< Padding for alignment */
  uint8_t  kernel_rsp_error_ptr[]; /**< This is an optional response payload containing pointers to Exception and Trace buffers (struct kernel_rsp_error_ptr_t)
                                    to provide user option to retrieve the respective contents when a Kernel execution fails */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_kernel_abort_cmd_t
    \brief Command to abort a currently running kernel on the device
*/
struct device_ops_kernel_abort_cmd_t {
  struct cmd_header_t command_info;
  uint16_t  kernel_launch_tag_id; /**< Tag ID of the kernel_launch command the we want to abort */
  uint8_t  pad[6]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_kernel_abort_rsp_t
    \brief Response to an abort request
*/
struct device_ops_kernel_abort_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_api_kernel_abort_response_e  status; /**< Kernel Abort status */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_dma_readlist_cmd_t
    \brief Single list Command to perform multiple DMA reads from device memory
*/
struct device_ops_dma_readlist_cmd_t {
  struct cmd_header_t command_info;
  struct dma_read_node  list[]; /**< Arrays of Structs containing Src/Dst/Size of numerous data transfers to be performed in a single command.
            Each entry is 1 Data transfer with its own Src/Dst/Size */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_dma_readlist_rsp_t
    \brief DMA readlist command response
*/
struct device_ops_dma_readlist_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Timestamp (in cycles) at which the command was dispatched */
  uint64_t  device_cmd_execute_dur; /**< Time transpired between command dispatch and command completion */
  uint64_t  device_cmd_wait_dur; /**< Time transpired between command arrival and dispatch */
  dev_ops_api_dma_response_e  status; /**< Status of the DMA operation */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_dma_writelist_cmd_t
    \brief Single list Command to perform multiple DMA writes on device memory
*/
struct device_ops_dma_writelist_cmd_t {
  struct cmd_header_t command_info;
  struct dma_write_node  list[]; /**< Arrays of Structs containing Src/Dst/Size of numerous data transfers to be performed in a single command.
            Each entry is 1 Data transfer with its own Src/Dst/Size */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_dma_writelist_rsp_t
    \brief DMA writelist command response
*/
struct device_ops_dma_writelist_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Timestamp (in cycles) at which the command was dispatched */
  uint64_t  device_cmd_execute_dur; /**< Time transpired between command dispatch and command completion */
  uint64_t  device_cmd_wait_dur; /**< Time transpired between command arrival and dispatch */
  dev_ops_api_dma_response_e  status; /**< Status of the DMA operation */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_p2pdma_readlist_cmd_t
    \brief Single list command to perform multiple P2P DMA read transfers. The read terminology is w.r.t to the peer
           partner device i.e. the peer partner (dst_device) will be reading from DRAM of main peer (src_device) and
           this command is to be sent to the src_device
*/
struct device_ops_p2pdma_readlist_cmd_t {
  struct cmd_header_t command_info;
  struct p2pdma_node  list[]; /**< Arrays of Structs containing Src/Dst/Size of numerous P2P data transfers to be performed in a single command.
            Each entry is 1 Data transfer with its own Src/Dst/Size and peer partner Device Id */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_p2pdma_readlist_rsp_t
    \brief P2P DMA readlist command response
*/
struct device_ops_p2pdma_readlist_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Timestamp (in cycles) at which the command was dispatched */
  uint64_t  device_cmd_execute_dur; /**< Time transpired between command dispatch and command completion */
  uint64_t  device_cmd_wait_dur; /**< Time transpired between command arrival and dispatch */
  dev_ops_api_dma_response_e  status; /**< Status of the DMA operation */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_p2pdma_writelist_cmd_t
    \brief Single list command to perform multiple P2P DMA write transfers. The write terminology is w.r.t to the peer
           partner device i.e. the peer partner (src_device) will be writing into DRAM of main peer (dst_device) and
           this command is to be sent to the dst_device
*/
struct device_ops_p2pdma_writelist_cmd_t {
  struct cmd_header_t command_info;
  struct p2pdma_node  list[]; /**< Arrays of Structs containing Src/Dst/Size of numerous data transfers to be performed in a single command.
            Each entry is 1 Data transfer with its own Src/Dst/Size and peer partner Device Id */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_p2pdma_writelist_rsp_t
    \brief P2P DMA writelist command response
*/
struct device_ops_p2pdma_writelist_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  uint64_t  device_cmd_start_ts; /**< Timestamp (in cycles) at which the command was dispatched */
  uint64_t  device_cmd_execute_dur; /**< Time transpired between command dispatch and command completion */
  uint64_t  device_cmd_wait_dur; /**< Time transpired between command arrival and dispatch */
  dev_ops_api_dma_response_e  status; /**< Status of the DMA operation */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_trace_rt_config_cmd_t
    \brief Configure the trace configuration
*/
struct device_ops_trace_rt_config_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  shire_mask; /**< Bit Mask of Shire to enable Trace Capture */
  uint64_t  thread_mask; /**< Bit Mask of Thread within a Shire to enable Trace Capture */
  uint32_t  event_mask; /**< This is a bit mask, each bit corresponds to a specific Event to trace */
  uint32_t  filter_mask; /**< This is a bit mask representing a list of filters for a given event to trace */
  uint32_t  threshold; /**< Trace buffer threshold, device will notify Host when buffer is filled up-to this threshold value. */
  uint32_t  pad; /**< Padding for alignment. */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_trace_rt_config_rsp_t
    \brief Trace configure command reply
*/
struct device_ops_trace_rt_config_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_trace_rt_config_response_e  status; /**< DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS or non-zero error code */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_trace_rt_control_cmd_t
    \brief Configure options to start, stop, OR fetch trace logs.
*/
struct device_ops_trace_rt_control_cmd_t {
  struct cmd_header_t command_info;
  uint32_t  rt_type; /**< This is a bit mask, each bit corresponds to a Trace RT component.
    Bit 0 - MM Trace Runtime buffer as per DIR:- MEM_OPS_MM_FW_TRACE
    Bit 1 - CM Trace Runtime buffer as per DIR:- MEM_OPS_CM_FW_TRACE */
  uint32_t  control; /**< This is a bit mask, each bit corresponds control a Trace RT control.
    Bit 0 - 0 - Disable - instructs device runtime to stop tracing
            1 - Enable - instructs device runtime to start tracing (Default)
    Bit 1 - 0 - Dump to Trace Buffer (Default)
            1 - Redirect to UART */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_trace_rt_control_rsp_t
    \brief Trace runtime control command reply
*/
struct device_ops_trace_rt_control_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_trace_rt_control_response_e  status; /**< DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS or non-zero error code */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_cm_reset_cmd_t
    \brief CM Reset command
*/
struct device_ops_cm_reset_cmd_t {
  struct cmd_header_t command_info;
  uint64_t  cm_shire_mask; /**< CM Shire Mask */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_cm_reset_rsp_t
    \brief CM Reset response
*/
struct device_ops_cm_reset_rsp_t {
  struct rsp_header_t response_info; /**< Response header */
  dev_ops_api_cm_reset_response_e  status; /**< CM Reset status */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_device_fw_error_t
    \brief Generic error reported by the device
*/
struct device_ops_device_fw_error_t {
  struct evt_header_t event_info;
  uint64_t  payload; /**< Optional payload as per error type */
  dev_ops_api_error_type_e  error_type; /**< Error Type */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_ops_trace_buffer_full_event_t
    \brief Event triggered when trace buffer has reached threshold or is full
*/
struct device_ops_trace_buffer_full_event_t {
  struct evt_header_t event_info;
  uint32_t  data_size; /**< Total data size when event is triggered */
  uint8_t  buffer_type; /**< Trace buffer type idenitifier */
  uint8_t  pad[3]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));


#endif /* ET_DEVICE_OPS_API_RPC_TYPES_H */
