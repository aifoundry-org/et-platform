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
 * Enumeration of all the RPC messages that the Device Mgmt/Ops API
 * send/receive
 */
enum device_msg_e {
	DEV_MGMT_API_MID_BEGIN = 0,
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
	DEV_MGMT_API_MID_CM_ETH_EVENT,
	DEV_MGMT_API_MID_CM_HTH_EVENT,
	DEV_MGMT_API_MID_THROTTLE_TIME_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_EXCEPTION_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_HANG_EVENT,
	DEV_MGMT_API_MID_SP_RUNTIME_ERROR_EVENT,
	/* Device Mgmt Event IDs reserved */
	DEV_MGMT_API_MID_EVENTS_END = 511,
	DEV_OPS_API_MID_BEGIN = 512,
	DEV_OPS_API_MID_COMPATIBILITY_CMD,
	DEV_OPS_API_MID_COMPATIBILITY_RSP,
	DEV_OPS_API_MID_FW_VERSION_CMD,
	DEV_OPS_API_MID_FW_VERSION_RSP,
	DEV_OPS_API_MID_ECHO_CMD,
	DEV_OPS_API_MID_ECHO_RSP,
	DEV_OPS_API_MID_KERNEL_LAUNCH_CMD,
	DEV_OPS_API_MID_KERNEL_LAUNCH_RSP,
	DEV_OPS_API_MID_KERNEL_ABORT_CMD,
	DEV_OPS_API_MID_KERNEL_ABORT_RSP,
	DEV_OPS_API_MID_DATA_READ_CMD,
	DEV_OPS_API_MID_DATA_READ_RSP,
	DEV_OPS_API_MID_DATA_WRITE_CMD,
	DEV_OPS_API_MID_DATA_WRITE_RSP,
	DEV_OPS_API_MID_DMA_READLIST_CMD,
	DEV_OPS_API_MID_DMA_READLIST_RSP,
	DEV_OPS_API_MID_DMA_WRITELIST_CMD,
	DEV_OPS_API_MID_DMA_WRITELIST_RSP,
	DEV_OPS_API_MID_DEVICE_FW_ERROR,
	/* Device Ops Message IDs reserved */
	DEV_OPS_API_MID_END = 1023,
};

/*
 * DMA response status enum
 */
enum dev_ops_api_dma_response_e {
	DEV_OPS_API_DMA_RESPONSE_COMPLETE = 0,
	DEV_OPS_API_DMA_RESPONSE_ERROR = 1,
	DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE = 3,
	DEV_OPS_API_DMA_RESPONSE_ABORTED = 4,
	DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG = 5,
	DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS = 6,
};

/*
 * Command to read data from device memory
 */
struct device_ops_data_read_cmd_t {
	struct cmd_header_t command_info;
	u64 dst_host_virt_addr;
	u64 dst_host_phy_addr;
	u64 src_device_phy_addr;
	u32 size;
	u32 pad;
} __packed __aligned(8);

/*
 * Data read command response
 */
struct device_ops_data_read_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __packed __aligned(8);

/*
 * Node containing one DMA read transfer information
 */
struct dma_read_node {
	u64 dst_host_virt_addr;
	u64 dst_host_phy_addr;
	u64 src_device_phy_addr;
	u32 size;
	u8 pad[4];
} __packed __aligned(8);

/*
 * Single list Command to perform multiple DMA reads from device memory
 */
struct device_ops_dma_readlist_cmd_t {
	struct cmd_header_t command_info;
	struct dma_read_node list[];
} __packed __aligned(8);

/*
 * DMA readlist command response
 */
struct device_ops_dma_readlist_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __packed __aligned(8);

/*
 * Command to write data to device memory
 */
struct device_ops_data_write_cmd_t {
	struct cmd_header_t command_info;
	u64 src_host_virt_addr;
	u64 src_host_phy_addr;
	u64 dst_device_phy_addr;
	u32 size;
	u32 pad;
} __packed __aligned(8);

/*
 * Data write command response
 */
struct device_ops_data_write_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __packed __aligned(8);

/*
 * Node containing one DMA write transfer information
 */
struct dma_write_node {
	u64 src_host_virt_addr;
	u64 src_host_phy_addr;
	u64 dst_device_phy_addr;
	u32 size;
	u8 pad[4];
} __packed __aligned(8);

/*
 * Single list Command to perform multiple DMA writes on device memory
 */
struct device_ops_dma_writelist_cmd_t {
	struct cmd_header_t command_info;
	struct dma_write_node list[];
} __packed __aligned(8);

/*
 * DMA writelist command response
 */
struct device_ops_dma_writelist_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __packed __aligned(8);

/*
 * Device to host event message
 */
struct device_mgmt_event_msg_t {
	struct cmn_header_t event_info;
	u16 class_count; // event class[1:0] and error count[15:2]
	u64 event_syndrome[2]; // hardware defined event syndrome
} __packed;

#endif // ET_DEVICE_API_H
