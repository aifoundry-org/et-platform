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
 * Enumeration of all the RPC messages that the Device Ops API send/receive
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
	DEV_MGMT_EID_THERMAL_HIGH,
	DEV_MGMT_EID_WDOG_TIMEOUT,
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

enum event_class {
	ECLASS_INFO,           /* Informational */
	ECLASS_WARNING,        /* Warning */
	ECLASS_CRITICAL,       /* Critical */
	ECLASS_FATAL,          /* Fatal */
};

/*!
 * @enum error_type
 * @brief Enum defining event/error type
 */
enum error_type {
	CORRECTABLE,
	UNCORRETABLE,
};

#define EVENT_CLASS_MASK	0x0003
#define EVENT_COUNT_MASK	0x3FFF

#define LEVEL_INFO		"Info"
#define LEVEL_WARN		"Warning"
#define LEVEL_CRITICAL		"Critical"
#define LEVEL_FATAL		"Fatal"

#define PCIE_UCE_RESERVED_MASK		(BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define PCIE_UCE_DATA_LINK_PROTOCOL_ERR_STATUS_MASK		BIT(4)
#define PCIE_UCE_SURPRISE_DOWN_ERR_STATUS_MASK			BIT(5)
#define PCIE_UCE_POISONED_TLP_ERR_STATUS_MASK			BIT(12)
#define PCIE_UCE_FLOW_CONTROL_PROTOCOL_ERR_STATUS_MASK		BIT(13)
#define PCIE_UCE_COMPLETION_TIMEOUT_ERR_STATUS_MASK		BIT(14)
#define PCIE_UCE_COMPLETION_ABORT_ERR_STATUS_MASK		BIT(15)
#define PCIE_UCE_UNEXPECTED_COMPLETION_ERR_STATUS_MASK		BIT(16)
#define PCIE_UCE_RECEIVER_OVERFLOW_ERR_STATUS_MASK		BIT(17)
#define PCIE_UCE_MALFORMED_TLP_ERR_STATUS_MASK			BIT(18)
#define PCIE_UCE_ECRC_ERR_STATUS_MASK				BIT(19)
#define PCIE_UCE_UNSUPPORTED_REQ_ERR_STATUS_MASK		BIT(20)
#define PCIE_UCE_INTERNAL_ERR_STATUS_MASK			BIT(22)

#define PCIE_CE_RECEIVER_ERR_STATUS_MASK			BIT(0)
#define PCIE_CE_BAD_TLP_ERR_STATUS_MASK				BIT(6)
#define PCIE_CE_BAD_DLLP_ERR_STATUS_MASK			BIT(7)
#define PCIE_CE_REPLAY_NUM_ROLLOVER_ERR_STATUS_MASK		BIT(8)
#define PCIE_CE_REPLAY_TIMER_TIMEOUT_ERR_STATUS_MASK		BIT(12)
#define PCIE_CE_ADVISORY_NONFATAL_ERR_STATUS_MASK		BIT(13)
#define PCIE_CE_CORRECTED_INTERNAL_ERR_STATUS_MASK		BIT(14)
#define PCIE_CE_HEADER_LOG_OVERFLOW_ERR_STATUS_MASK		BIT(15)

#define SYNDROME_TEMP_MASK		0x3FULL
#define SYNDROME_TEMP_FRACTION_MASK	0x03ULL

#define CM_USER_KERNEL_ERROR		0
#define CM_RUNTIME_ERROR		1
#define MM_DISPATCHER_ERROR		2
#define MM_SQW_ERROR			3
#define MM_DMW_ERROR			4
#define MM_KW_ERROR			5

#define GET_ESR_SC_ERR_LOG_INFO_V_BIT(reg)		\
	((reg)  & 0x0000000000000001ULL)
#define GET_ESR_SC_ERR_LOG_INFO_M_BIT(reg)		\
	(((reg) & 0x0000000000000002ULL) >> 1)
#define GET_ESR_SC_ERR_LOG_INFO_E_BIT(reg)		\
	(((reg) & 0x0000000000000004ULL) >> 2)
#define GET_ESR_SC_ERR_LOG_INFO_I_BIT(reg)		\
	(((reg) & 0x00000000000000008LL) >> 3)
#define GET_ESR_SC_ERR_LOG_INFO_CODE_BITS(reg)		\
	(((reg) & 0x00000000000000F0ULL) >> 4)
#define GET_ESR_SC_ERR_LOG_INFO_INDEX_BITS(reg)		\
	(((reg) & 0x000000FFFFFFFF00ULL) >> 8)
#define GET_ESR_SC_ERR_LOG_INFO_ERR_BITS(reg)		\
	(((reg) & 0x0000FF0000000000ULL) >> 40)
#define GET_ESR_SC_ERR_LOG_INFO_RAM_BITS(reg)		\
	(((reg) & 0x000F000000000000ULL) >> 48)

#define GET_OVER_THROTTLE_DURATION_BITS(reg)		\
	((reg) & 0x00000000FFFFFFFFULL)

/*
 * Device to host event message
 */
struct device_mgmt_event_msg_t {
	struct cmn_header_t event_info;
	u16 class_count; // event class[1:0] and error count[15:2]
	u64 event_syndrome[2]; // hardware defined event syndrome
} __attribute__((__packed__));

/**
 * struct event_dbg_msg - Debug info based on event message
 * @level:	Severity level: warning, critical or fatal
 * @desc:	Text describing the event
 * @count:	No. of times an event occurred
 * @syndrome:	Info to debug or identify the event source
 */
struct event_dbg_msg {
	char *level;
	char *desc;
	u16 count;
	char *syndrome;
};

#endif // ET_DEVICE_API_H
