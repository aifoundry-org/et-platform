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
} __attribute__ ((packed));

/*
 * Command header for all commands host to device
 */
struct cmd_header_t {
	struct cmn_header_t cmd_hdr;
	u16 flags;
} __attribute__ ((packed, aligned(8)));

/*
 * Response header for all command responses from device to host
 */
struct rsp_header_t {
	struct cmn_header_t rsp_hdr;
	u8 pad[2];
} __attribute__ ((packed, aligned(8)));

/*
 * Event header for all events from device to host
 */
struct evt_header_t {
	u16 size;
	u16 evt_id;
	u8 pad[4];
} __attribute__ ((packed, aligned(8)));

/*
 * Enumeration of all the RPC messages that the Device Mgmt/Ops API
 * send/receive
 */
enum device_msg_e {
	DEV_OPS_API_MID_BEGIN = 0,
	DEV_OPS_API_MID_NONE = 0,
	DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR,
	DEV_OPS_API_MID_LAST,
	/* Ops API Msg IDs reserved for future use */
	DEV_OPS_API_MID_END = 255,
	DEV_MGMT_EID_BEGIN = 256,
	DEV_MGMT_EID_PCIE_CE = 256,
	DEV_MGMT_EID_PCIE_UCE,
	DEV_MGMT_EID_DRAM_CE,
	DEV_MGMT_EID_DRAM_UCE,
	DEV_MGMT_EID_SRAM_CE,
	DEV_MGMT_EID_SRAM_UCE,
	DEV_MGMT_EID_THERMAL_LOW,
	DEV_MGMT_EID_PMIC_ERROR,
	DEV_MGMT_EID_WDOG_INTERNAL_TIMEOUT,
	DEV_MGMT_EID_WDOG_EXTERNAL_TIMEOUT,
	DEV_MGMT_EID_FW_BOOT,
	DEV_MGMT_EID_CM_ETH,
	DEV_MGMT_EID_CM_HTH,
	DEV_MGMT_EID_THROTTLE_TIME,
	/* Mgmt Event IDs reserved for future use */
	DEV_MGMT_EID_END = 512
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
} __attribute__ ((packed, aligned(8)));

/*
 * Data read command response
 */
struct device_ops_data_read_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __attribute__ ((packed, aligned(8)));

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
} __attribute__ ((packed, aligned(8)));

/*
 * Data write command response
 */
struct device_ops_data_write_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
	u32 pad;
} __attribute__ ((packed, aligned(8)));

/*
 * Device to host event message
 */
struct device_mgmt_event_msg_t {
	struct cmn_header_t event_info;
	u16 class_count; // event class[1:0] and error count[15:2]
	u64 event_syndrome[2]; // hardware defined event syndrome
} __attribute__((__packed__));

#endif // ET_DEVICE_API_H
