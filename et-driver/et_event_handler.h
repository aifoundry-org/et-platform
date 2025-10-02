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

/**
 * @enum event_class - Enum defining event/error class
 * @ECLASS_INFO: Informational
 * @ECLASS_WARNING: Warning
 * @ECLASS_CRITICAL: Critical
 * @ECLASS_FATAL: Fatal
 */
enum event_class { ECLASS_INFO, ECLASS_WARNING, ECLASS_CRITICAL, ECLASS_FATAL };

/**
 * enum error_type - Enum defining event/error type
 */
enum error_type { CORRECTABLE, UNCORRETABLE };

// clang-format off

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
#define PMIC_ERROR_REGULATOR_FAULT_INT_MASK	        BIT(7)

#define DRAM_CYC_COUNT_OVERFLOW_INT_MASK            BIT(15)
#define DRAM_PERF_COUNT1_OVERFLOW_INT_MASK          BIT(14)
#define DRAM_PERF_COUNT0_OVERFLOW_INT_MASK          BIT(13)
#define DRAM_MC1_DERATE_TEMP_LIMIT_INT_MASK         BIT(12)
#define DRAM_MC0_DERATE_TEMP_LIMIT_INT_MASK         BIT(11)
#define DRAM_MC1_SBR_DONE_INT_MASK                  BIT(10)
#define DRAM_MC0_SBR_DONE_INT_MASK                  BIT(9)
#define DRAM_MC1_ECC_CORRECTED_ERR_INT_MASK         BIT(8)
#define DRAM_MC0_ECC_CORRECTED_ERR_INT_MASK         BIT(7)
#define DRAM_DDRPHY_INT_MASK                        BIT(6)
#define DRAM_DFI1_ERR_INT_MASK                      BIT(5)
#define DRAM_DFI0_ERR_INT_MASK                      BIT(4)
#define DRAM_MC1_DFI_ALERT_ERR_INT_MASK             BIT(3)
#define DRAM_MC0_DFI_ALERT_ERR_INT_MASK             BIT(2)
#define DRAM_MC1_ECC_UNCORRECTED_ERR_INT_MASK       BIT(1)
#define DRAM_MC0_ECC_UNCORRECTED_ERR_INT_MASK       BIT(0)

#define GET_MEMSHIRE_BITS(x)		\
	((x) >> 32 & 0x07ULL)

#define GET_ECCADDR0_RANK_BITS(x)	\
	(((x) & 0x00000000FF000000ULL) >> 24)
#define GET_ECCADDR0_ROW_BITS(x)	\
	((x) & 0x0000000000FFFFFFFUL)
#define GET_ECCADDR1_CID_BITS(x)	\
	(((x) & (0x00000000F0000000ULL << 32)) >> (28 + 32))
#define GET_ECCADDR1_BG_BITS(x)		\
	(((x) & (0x000000000F000000ULL << 32)) >> (24 + 32))
#define GET_ECCADDR1_BANK_BITS(x)	\
	(((x) & (0x0000000000FF0000ULL << 32)) >> (16 + 32))
#define GET_ECCADDR1_BLOCK_BITS(x)	\
	(((x) & (0x0000000000000FFFULL << 32)) >> 32)

#define SYNDROME_TEMP_CURR_MASK	0x00000000000000FFULL
#define SYNDROME_TEMP_THR_MASK	0x00000000000000FFULL

#define SYNDROME_PWR_CURR_MASK	0x00000000FFFFFFFFULL
#define SYNDROME_PWR_THR_MASK	0x00000000FFFFFFFFULL

#define SYNDROME_TEMP_MASK		0x000000000000003FULL
#define SYNDROME_TEMP_FRACT_MASK	0x0000000000000003ULL

#define SYNDROME_PWR_MASK		0x000000000000003FULL
#define SYNDROME_PWR_FRACT_MASK		0x0000000000000003ULL

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

// clang-format on

/**
 * enum sp2mm_error_type - Enum defining types of error reported by MM to SP
 *
 * NOTE: Must be kept in sync with the device
 */
enum sp2mm_error_type {
	SP_RECOVERABLE_FW_MM_HANG = 0,
	SP_RECOVERABLE_FW_MM_ERROR,
	MM_RECOVERABLE_FW_CM_RUNTIME_ERROR,
	MM_RECOVERABLE_FW_MM_DISPATCHER_ERROR,
	MM_RECOVERABLE_FW_MM_DMAW_ERROR,
	MM_RECOVERABLE_FW_MM_SQW_ERROR,
	MM_RECOVERABLE_FW_MM_SQW_HP_ERROR,
	MM_RECOVERABLE_FW_MM_SPW_ERROR,
	MM_RECOVERABLE_FW_MM_KW_ERROR,
	MM_RECOVERABLE_OPS_API_KERNEL_LAUNCH,
	MM_RECOVERABLE_OPS_API_DMA_READLIST,
	MM_RECOVERABLE_OPS_API_DMA_WRITELIST,
	MM_RECOVERABLE_OPS_API_ECHO,
	MM_RECOVERABLE_OPS_API_FW_VER,
	MM_RECOVERABLE_OPS_API_COMPATIBILITY,
	MM_RECOVERABLE_OPS_API_ABORT,
	MM_RECOVERABLE_OPS_API_CM_RESET,
	MM_RECOVERABLE_OPS_API_TRACE_RT_CONFIG,
	MM_RECOVERABLE_OPS_API_TRACE_RT_CONTROL
};

/**
 * struct event_dbg_msg - Debug info based on event message
 * @level: Severity level: warning, critical or fatal
 * @desc: Text describing the event
 * @count: No. of times an event occurred
 * @syndrome: Info to debug or identify the event source
 */
struct event_dbg_msg {
	char *level;
	char *desc;
	u16 count;
	char *syndrome;
};

#define ET_EVENT_SYNDROME_LEN 960

/**
 * et_print_event() - Print error event in dmesg
 * @pdev: Pointer to struct pci_dev
 * @dbg_msg: Pointer to struct event_dbg_msg, containing print message info.
 */
static inline void et_print_event(struct pci_dev *pdev,
				  struct event_dbg_msg *dbg_msg)
{
	dev_info(&pdev->dev,
		 "Error Event Detected\n"
		 "Level     : %s\n"
		 "Desc      : %s\n"
		 "Count     : %d\n"
		 "Syndrome  : %s",
		 dbg_msg->level, dbg_msg->desc, dbg_msg->count,
		 dbg_msg->syndrome);
}

int et_handle_device_event(struct et_cqueue *cq,
			   struct device_mgmt_event_msg_t *event_msg);

#endif
