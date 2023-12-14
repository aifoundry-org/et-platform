/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

#ifndef ET_DEVICE_API_H
#define ET_DEVICE_API_H

#include <linux/types.h>

/**
 * struct cmn_header_t - Header common to all device-api messages
 * @size: Total size of the device-api message
 * @tag_id: unique tag_id identifier to correlate commands and responses
 * @msg_id: unique msg_id identifier for the type of the message
 * @flags: any flags to pass-on
 */
struct cmn_header_t {
	u16 size;
	u16 tag_id;
	u16 msg_id;
	u16 flags;
} __packed __aligned(8);

/**
 * struct cmd_header_t - Command header for host to device commands
 *
 * Same as cmn_header_t structure
 */
struct cmd_header_t {
	struct cmn_header_t cmd_hdr;
} __packed __aligned(8);

/**
 * struct rsp_header_t - Response header for device to host responses
 *
 * Same as cmn_header_t structure. Sent by device in result of the sent command
 */
struct rsp_header_t {
	struct cmn_header_t rsp_hdr;
} __packed __aligned(8);

/**
 * struct evt_header_t - Event header for device to host events
 *
 * Same as cmn_header_t structure
 */
struct evt_header_t {
	struct cmn_header_t evt_hdr;
} __packed __aligned(8);

/**
 * enum device_mgmt_api_msg_e - Message IDs for the Device Mgmt API
 */
enum device_mgmt_api_msg_e {
	DEV_MGMT_API_MID_BEGIN = 0,
	/* Device Mgmt Message IDs reserved */
	DEV_MGMT_API_MID_ETSOC_RESET = 52,
	DEV_MGMT_API_MID_MM_RESET = 64,
	/* Device Mgmt Message IDs reserved */
	DEV_MGMT_API_MID_EVENTS_BEGIN = 256,
	DEV_MGMT_API_MID_PCIE_CE_EVENT = 256,
	DEV_MGMT_API_MID_PCIE_UCE_EVENT,
	DEV_MGMT_API_MID_DRAM_CE_EVENT,
	DEV_MGMT_API_MID_DRAM_UCE_EVENT,
	DEV_MGMT_API_MID_SRAM_CE_EVENT,
	DEV_MGMT_API_MID_SRAM_UCE_EVENT,
	DEV_MGMT_API_MID_THERMAL_LOW_EVENT,
	DEV_MGMT_API_MID_PMIC_ERROR_EVENT,
	DEV_MGMT_API_MID_FW_BOOT_EVENT,
	DEV_MGMT_API_MID_MINION_RUNTIME_ERROR_EVENT,
	DEV_MGMT_API_MID_MINION_RUNTIME_HANG_EVENT,
	DEV_MGMT_API_MID_THROTTLE_TIME_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_EXCEPTION_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_HANG_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_ERROR_EVENT,
	DEV_MGMT_API_MID_SP_WATCHDOG_RESET_EVENT,
	DEV_MGMT_API_MID_SP_TRACE_BUFFER_FULL_EVENT,
	/* Device Mgmt Event IDs reserved */
	DEV_MGMT_API_MID_EVENTS_END = 511,
};

/**
 * enum dev_mgmt_api_mm_reset_response_e - MM reset response status enum
 */
enum dev_mgmt_api_mm_reset_response_e {
	DEV_OPS_API_MM_RESET_RESPONSE_COMPLETE = 0,
	// TODO: SW-11288: Add other possible error responses
};

/**
 * struct dma_node - Node containing one DMA transfer information
 * @host_virt_addr: Host virtual address
 * @host_phys_addr: Host physical address to be used by DMA engine
 * @device_phys_addr: Device SOC address to be used by DMA engine
 * @size: Size of DMA transfer in bytes
 * @pad: Padding for alignment
 */
struct dma_node {
	u64 host_virt_addr;
	u64 host_phys_addr;
	u64 device_phys_addr;
	u32 size;
	u8 pad[4];
} __packed __aligned(8);

/**
 * struct device_ops_dma_list_cmd_t - List Command for multiple DMAs
 * @command_info: Command header information
 * list: Flexible array of struct dma_node
 *
 * List command can perform multiple DMA read OR write operations to/from
 * device memory in a single command
 *
 * NOTE: DMA readlist/writelist commands are internally consolidated in driver
 * but device-api has separate commands as of now. Hence, both commands should
 * be kept identical in device-api.
 */
struct device_ops_dma_list_cmd_t {
	struct cmd_header_t command_info;
	struct dma_node list[];
} __packed __aligned(8);

/**
 * struct p2pdma_node - Node containing one P2PDMA transfer information
 * @peer_device_phys_addr: Peer device SOC address
 * @peer_device_bus_addr: Peer device DMA bus address to be used by DMA engine
 * @this_device_phys_addr: This device SOC address to be used by DMA engine
 * @size: Size of P2P DMA transfer in bytes
 * @peer_devnum: Peer device enumeration ID, the device node index
 * @pad: Padding for alignment
 */
struct p2pdma_node {
	u64 peer_device_phys_addr;
	u64 peer_device_bus_addr;
	u64 this_device_phys_addr;
	u32 size;
	u16 peer_devnum;
	u8 pad[2];
} __packed __aligned(8);

/**
 * struct device_ops_p2pdma_list_cmd_t - List Command for multiple P2P DMAs
 * @command_info: Command header information
 * @list: Flexible array of struct p2pdma_node
 *
 * List command can perform multiple P2P DMA read OR write operation to/from
 * main device memory from/to peer device memory in a single command
 *
 * NOTE: P2P DMA readlist/writelist commands are internally consolidated in
 * driver but device-api has separate commands as of now. Hence, both commands
 * should be kept identical in device-api.
 */
struct device_ops_p2pdma_list_cmd_t {
	struct cmd_header_t command_info;
	struct p2pdma_node list[];
} __packed __aligned(8);

/**
 * struct device_ops_dma_list_rsp_t - DMA/P2PDMA list command response
 * @response_info: Response header information
 * @cmd_start_ts: Commands starting timestamp
 * @cmd_execution_time: Execution time duration cycles
 * @cmd_wait_time: Wait time duration cycles
 * @status: Command result status
 * @pad: Padding for alignment
 */
struct device_ops_dma_list_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_start_ts;
	u64 cmd_execution_time;
	u64 cmd_wait_time;
	u32 status;
	u8 pad[4];
} __packed __aligned(8);

/**
 * struct device_mgmt_rsp_hdr_t - Device Mgmt to host response message header
 * @response_info: Response header information
 * @device_latency_usec: device execution duration in microseconds
 * @status: Command result status
 * @pad: Padding for alignment
 */
struct device_mgmt_rsp_hdr_t {
	struct rsp_header_t response_info;
	u64 device_latency_usec;
	s32 status;
	u32 pad;
} __packed __aligned(8);

/**
 * struct device_mgmt_event_msg_t - Mgmt Device to host event message
 * @event_info: Event header information
 * @class_count: event class[1:0] and error count[15:2]
 * @event_syndrome: hardware defined event syndrome
 */
struct device_mgmt_event_msg_t {
	struct cmn_header_t event_info;
	u16 class_count;
	u64 event_syndrome[2];
} __packed __aligned(8);

#endif // ET_DEVICE_API_H
