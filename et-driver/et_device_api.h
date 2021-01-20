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

/*
 * Common header, common to command, response, and events
 */
struct cmn_header_t {
	u16 size;
	u16 tag_id;
	u16 msg_id;
};

/*
 * Command header for all commands host to device
 */
struct cmd_header_t {
	struct cmn_header_t cmd_hdr;
	u16 flags;
};

/*
 * Response header for all command responses from device to host
 */
struct rsp_header_t {
	struct cmn_header_t rsp_hdr;
};

/*
 * Event header for all events from device to host
 */
struct evt_header_t {
	struct cmn_header_t evt_hdr;
};

/*
 * Enumeration of all the RPC messages that the Device Ops API send/receive
 */
enum device_ops_api_msg_e {
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
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD,
	DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP,
	DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR,
	DEV_OPS_API_MID_LAST
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
};

/*
 * Data read command response
 */
struct device_ops_data_read_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
};

/*
 * Command to write data to device memory
 */
struct device_ops_data_write_cmd_t {
	struct cmd_header_t command_info;
	u64 src_host_virt_addr;
	u64 src_host_phy_addr;
	u64 dst_device_phy_addr;
	u32 size;
};

/*
 * Data write command response
 */
struct device_ops_data_write_rsp_t {
	struct rsp_header_t response_info;
	u64 cmd_wait_time;
	u64 cmd_execution_time;
	u32 status;
};

#endif // ET_DEVICE_API_H
