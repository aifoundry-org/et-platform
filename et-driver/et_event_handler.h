/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (C) 2020 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

#ifndef __ET_EVENT_HANDLER_H
#define __ET_EVENT_HANDLER_H

#include <linux/pci.h>

#include "et_device_api.h"
#include "et_vqueue.h"

// clang-format off
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

struct asic_frequencies {
	u32 minion_shire_mhz;
	u32 noc_mhz;
	u32 mem_shire_mhz;
	u32 ddr_mhz;
	u32 pcie_shire_mhz;
	u32 io_shire_mhz;
} __packed;

struct dram_bw {
	u32 read_req_sec;
	u32 write_req_sec;
} __packed;

struct max_dram_bw {
	u8 max_bw_rd_req_sec;
	u8 max_bw_wr_req_sec;
	u8 pad[6];
} __packed;

struct module_uptime {
	u16 day;
	u8 hours;
	u8 mins;
	u8 pad[4];
} __packed;

struct module_voltage {
	u8 ddr;
	u8 l2_cache;
	u8 maxion;
	u8 minion;
	u8 pcie;
	u8 noc;
	u8 pcie_logic;
	u8 vddqlp;
	u8 vddq;
	u8 pad[7];
} __packed;

typedef u8 power_state_e;

#define SP_GPR_REGISTERS		28
#define SP_CSR_REGISTERS		4
#define SP_NUM_REGISTERS		(SP_GPR_REGISTERS + SP_CSR_REGISTERS)
#define SP_EXCEPTION_STACK_FRAME_SIZE	(sizeof(u64) * SP_GPR_REGISTERS)
#define SP_EXCEPTION_CSRS_FRAME_SIZE	(sizeof(u64) * 4)
#define SP_EXCEPTION_FRAME_SIZE		(SP_EXCEPTION_STACK_FRAME_SIZE +       \
					SP_EXCEPTION_CSRS_FRAME_SIZE)
#define SP_PERF_GLOBALS_SIZE		(sizeof(struct asic_frequencies) +     \
					sizeof(struct dram_bw) +               \
					sizeof(struct max_dram_bw) +           \
					sizeof(u32) + sizeof(u64))
#define SP_POWER_GLOBALS_SIZE		(sizeof(power_state_e) +               \
					sizeof(u8) + (sizeof(u8) * 3) +        \
					sizeof(struct module_uptime) +         \
					sizeof(struct module_voltage) +        \
					sizeof(u64) + sizeof(u64))
#define SP_GLOBALS_SIZE		(SP_PERF_GLOBALS_SIZE + SP_POWER_GLOBALS_SIZE)

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

#define PMIC_ERROR_OVER_TEMP_INT_MASK				BIT(0)
#define PMIC_ERROR_OVER_POWER_INT_MASK				BIT(1)
#define PMIC_ERROR_INPUT_VOLTAGE_TOO_LOW_INT_MASK		BIT(2)
#define PMIC_ERROR_MINION_REGULATOR_VOLTAGE_TOO_LOW_INT_MASK	BIT(3)
#define PMIC_ERROR_RESERVED_FOR_FUTURE_USE_MASK			BIT(4)
#define PMIC_ERROR_WRONG_MSG_FORMAT_OR_DATA_OUT_OF_BOUND_MASK	BIT(5)
#define PMIC_ERROR_REGULATOR_COMM_FAILED_OR_BAD_DATA_MASK	BIT(6)

#define SYNDROME_TEMP_MASK		0x000000000000003FULL
#define SYNDROME_TEMP_FRACT_MASK	0x0000000000000003ULL

#define SYNDROME_PWR_MASK		0x000000000000003FULL
#define SYNDROME_PWR_FRACT_MASK		0x0000000000000003ULL

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

#define ET_EVENT_SYNDROME_LEN			840

static inline void et_print_event(struct pci_dev *pdev,
				  struct event_dbg_msg *dbg_msg) {
	dev_info(&pdev->dev,
		 "Error Event Detected\nLevel     : %s\nDesc      : %s\nCount     : %d\nSyndrome  : %s",
		 dbg_msg->level,
		 dbg_msg->desc,
		 dbg_msg->count,
		 dbg_msg->syndrome);
}

int et_handle_device_event(struct et_cqueue *cq,
			   struct device_mgmt_event_msg_t *event_msg);

// clang-format on

#endif
