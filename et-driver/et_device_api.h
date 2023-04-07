/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-----------------------------------------------------------------------------
 */

#ifndef ET_DEVICE_API_H
#define ET_DEVICE_API_H

#include <linux/types.h>

/*
 * Common header, common to command, response, and events
 */
struct cmn_header_t {
	u16 size;
	u16 tag_id;
	u16 msg_id;
	u16 flags;
} __packed __aligned(8);

/*
 * Command header for all commands host to device
 */
struct cmd_header_t {
	struct cmn_header_t cmd_hdr;
} __packed __aligned(8);

/*
 * Response header for all command responses from device to host
 */
struct rsp_header_t {
	struct cmn_header_t rsp_hdr;
} __packed __aligned(8);

/*
 * Event header for all events from device to host
 */
struct evt_header_t {
	u16 size;
	u16 evt_id;
	u8 pad[4];
} __packed __aligned(8);

/*
 * Enumeration of all the RPC messages that the Device Mgmt API send/receive
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

/*
 * MM reset response status enum
 */
enum dev_mgmt_api_mm_reset_response_e {
	DEV_OPS_API_MM_RESET_RESPONSE_COMPLETE = 0,
	// TODO: SW-11288: Add other possible error responses
};

/*
 * Node containing one DMA transfer information
 */
struct dma_node {
	u64 host_virt_addr;
	u64 host_phys_addr;
	u64 device_phys_addr;
	u32 size;
	u8 pad[4];
} __packed __aligned(8);

/*
 * Single list Command to perform multiple DMA to/from device memory
 * NOTE: DMA readlist/writelist commands are internally consolidated in driver
 * but device-api has separate commands as of now. Hence, both commands should
 * be kept identical in device-api.
 */
struct device_ops_dma_list_cmd_t {
	struct cmd_header_t command_info;
	struct dma_node list[];
} __packed __aligned(8);

/*
 * Node containing one P2PDMA transfer information
 */
struct p2pdma_node {
	u64 peer_device_phys_addr;
	u64 peer_device_bus_addr;
	u64 this_device_phys_addr;
	u32 size;
	u16 peer_devnum;
	u8 pad[2];
} __packed __aligned(8);

/*
 * Single list Command to perform multiple P2PDMA to/from main device memory
 * from/to peer device memory
 * NOTE: P2P DMA readlist/writelist commands are internally consolidated in
 * driver but device-api has separate commands as of now. Hence, both commands
 * should be kept identical in device-api.
 */
struct device_ops_p2pdma_list_cmd_t {
	struct cmd_header_t command_info;
	struct p2pdma_node list[];
} __packed __aligned(8);

/*
 * DMA/P2PDMA list command response
 */
struct device_ops_dma_list_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_start_ts;
	u64 cmd_execution_time;
	u64 cmd_wait_time;
	u32 status;
	u8 pad[4];
} __packed __aligned(8);

/*
 * Device Mgmt to host response message header
 */
struct device_mgmt_rsp_hdr_t {
	struct rsp_header_t response_info;
	u64 device_latency_usec;
	s32 status;
	u32 pad;
} __packed __aligned(8);

/*
 * Device to host event message
 */
struct device_mgmt_event_msg_t {
	struct cmn_header_t event_info;
	u16 class_count; // event class[1:0] and error count[15:2]
	u64 event_syndrome[2]; // hardware defined event syndrome
} __packed __aligned(8);

#endif // ET_DEVICE_API_H
