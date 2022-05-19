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

#include "et_event_handler.h"
#include "et_circbuffer.h"
#include "et_io.h"
#include "et_pci_dev.h"
#include "et_trace.h"

#define VALUE_STR_MAX_LEN 128

static void parse_pcie_syndrome(struct device_mgmt_event_msg_t *event_msg,
				struct event_dbg_msg *dbg_msg)
{
	if (event_msg->event_info.msg_id == DEV_MGMT_API_MID_PCIE_CE_EVENT) {
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_RECEIVER_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Receiver Error Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_BAD_TLP_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Bad TLP Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_BAD_DLLP_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Bad DLLP Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_REPLAY_NUM_ROLLOVER_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Replay No Rollover Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_REPLAY_TIMER_TIMEOUT_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Replay Timer Timeout Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_ADVISORY_NONFATAL_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Advisory Non-fatal Error Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_CORRECTED_INTERNAL_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Corrected Internal Error Status\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_CE_HEADER_LOG_OVERFLOW_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Header Log Overflow Error Status\n");
	} else {
		if (event_msg->event_syndrome[0] & PCIE_UCE_RESERVED_MASK)
			strcat(dbg_msg->syndrome, "Reserved Bits\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_DATA_LINK_PROTOCOL_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Data Link Protocol Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_SURPRISE_DOWN_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Surprise Down Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_POISONED_TLP_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Poisoned TLP Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_FLOW_CONTROL_PROTOCOL_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Flow Ctrl Protocol Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_COMPLETION_TIMEOUT_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Completion Timeout Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_COMPLETION_ABORT_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Completion Abort Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_UNEXPECTED_COMPLETION_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Unexpected Completion Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_RECEIVER_OVERFLOW_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Receiver Overflow Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_MALFORMED_TLP_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Malformed TLP Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_ECRC_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "ECRC Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_UNSUPPORTED_REQ_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome,
			       "Unsupported Request Error\n");
		if (event_msg->event_syndrome[0] &
		    PCIE_UCE_INTERNAL_ERR_STATUS_MASK)
			strcat(dbg_msg->syndrome, "Internal Error\n");
	}
}

static void parse_dram_syndrome(struct device_mgmt_event_msg_t *event_msg,
				struct event_dbg_msg *dbg_msg)
{
	uint32_t memshire = GET_MEMSHIRE_BITS(event_msg->event_syndrome[0]);
	uint32_t controller_id;
	char value_str[VALUE_STR_MAX_LEN];

	if (event_msg->event_info.msg_id == DEV_MGMT_API_MID_DRAM_CE_EVENT) {
		if (event_msg->event_syndrome[0] &
		    DRAM_MC1_ECC_CORRECTED_ERR_INT_MASK)
			controller_id = 1;
		if (event_msg->event_syndrome[0] &
		    DRAM_MC0_ECC_CORRECTED_ERR_INT_MASK)
			controller_id = 0;

		snprintf(
			value_str,
			VALUE_STR_MAX_LEN,
			"Memory controller %d correctable ECC error in Memshire %d\n",
			controller_id,
			memshire);
		strcat(dbg_msg->syndrome, value_str);
	}
	if (event_msg->event_info.msg_id == DEV_MGMT_API_MID_DRAM_UCE_EVENT) {
		if (event_msg->event_syndrome[0] &
		    DRAM_MC1_ECC_UNCORRECTED_ERR_INT_MASK)
			controller_id = 1;
		if (event_msg->event_syndrome[0] &
		    DRAM_MC0_ECC_UNCORRECTED_ERR_INT_MASK)
			controller_id = 0;

		snprintf(
			value_str,
			VALUE_STR_MAX_LEN,
			"Memory controller %d uncorrectable ECC error in Memshire %d\n",
			controller_id,
			memshire);
		strcat(dbg_msg->syndrome, value_str);
	}

	snprintf(
		value_str,
		VALUE_STR_MAX_LEN,
		"Rank      : 0x%x\nRow       : 0x%x\nCID       : 0x%x\nBank Group: "
		"0x%x\nBank      : 0x%x\nBlock     : 0x%x",
		(int)GET_ECCADDR0_RANK_BITS(event_msg->event_syndrome[1]),
		(int)GET_ECCADDR0_ROW_BITS(event_msg->event_syndrome[1]),
		(int)GET_ECCADDR1_CID_BITS(event_msg->event_syndrome[1]),
		(int)GET_ECCADDR1_BG_BITS(event_msg->event_syndrome[1]),
		(int)GET_ECCADDR1_BANK_BITS(event_msg->event_syndrome[1]),
		(int)GET_ECCADDR1_BLOCK_BITS(event_msg->event_syndrome[1]));

	strcat(dbg_msg->syndrome, value_str);
}

static void parse_sram_syndrome(struct device_mgmt_event_msg_t *event_msg,
				struct event_dbg_msg *dbg_msg)
{
	sprintf(dbg_msg->syndrome,
		"ESR_SC_ERR_LOG_INFO\nValid     : %d\nMultiple  : %d\nEnabled   : %d\nImprecise : %d\nCode      : %d\nIndex     : %d\nError_bits: %d\nRam       : %d",
		(int)GET_ESR_SC_ERR_LOG_INFO_V_BIT(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_M_BIT(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_E_BIT(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_I_BIT(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_CODE_BITS(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_INDEX_BITS(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_ERR_BITS(
			event_msg->event_syndrome[0]),
		(int)GET_ESR_SC_ERR_LOG_INFO_RAM_BITS(
			event_msg->event_syndrome[0]));
}

static void parse_pmic_syndrome(struct device_mgmt_event_msg_t *event_msg,
				struct event_dbg_msg *dbg_msg)
{
	int value_whole;
	int value_fract;
	char value_str[VALUE_STR_MAX_LEN];

	if (event_msg->event_syndrome[0] & PMIC_ERROR_OVER_TEMP_INT_MASK) {
		value_fract = 25 * (event_msg->event_syndrome[1] &
				    SYNDROME_TEMP_FRACT_MASK);
		value_whole = (event_msg->event_syndrome[1] >> 2) &
			      SYNDROME_TEMP_MASK;
		snprintf(value_str,
			 VALUE_STR_MAX_LEN,
			 "Temperature Overshoot Beyond Threshold: %d.%d C\n",
			 value_whole,
			 value_fract);
		strcat(dbg_msg->syndrome, value_str);
	}
	if (event_msg->event_syndrome[0] & PMIC_ERROR_OVER_POWER_INT_MASK) {
		value_fract = 25 * ((event_msg->event_syndrome[1] >> 8) &
				    SYNDROME_PWR_FRACT_MASK);
		value_whole = (event_msg->event_syndrome[1] >> 10) &
			      SYNDROME_PWR_MASK;
		snprintf(value_str,
			 VALUE_STR_MAX_LEN,
			 "Power Overshoot Beyond Threshold: %d.%d W\n",
			 value_whole,
			 value_fract);
		strcat(dbg_msg->syndrome, value_str);
	}
	if (event_msg->event_syndrome[0] &
	    PMIC_ERROR_INPUT_VOLTAGE_TOO_LOW_INT_MASK)
		strcat(dbg_msg->syndrome, "Input Voltage Too Low Interrupt\n");
	if (event_msg->event_syndrome[0] &
	    PMIC_ERROR_MINION_REGULATOR_VOLTAGE_TOO_LOW_INT_MASK)
		strcat(dbg_msg->syndrome,
		       "Minion Regulator Voltage Too Low Interrupt\n");
	if (event_msg->event_syndrome[0] &
	    PMIC_ERROR_RESERVED_FOR_FUTURE_USE_MASK)
		strcat(dbg_msg->syndrome, "Reserved For Future Use\n");
	if (event_msg->event_syndrome[0] &
	    PMIC_ERROR_WRONG_MSG_FORMAT_OR_DATA_OUT_OF_BOUND_MASK)
		strcat(dbg_msg->syndrome,
		       "Wrong Message Format or Data Out Of Bound\n");
	if (event_msg->event_syndrome[0] &
	    PMIC_ERROR_REGULATOR_COMM_FAILED_OR_BAD_DATA_MASK)
		strcat(dbg_msg->syndrome,
		       "Communication With Regulator Failed or Bad Data\n");
}

static void parse_thermal_syndrome(struct device_mgmt_event_msg_t *event_msg,
				   struct event_dbg_msg *dbg_msg)
{
	int temp_whole;
	int temp_fract;

	temp_fract =
		25 * (event_msg->event_syndrome[0] & SYNDROME_TEMP_FRACT_MASK);
	temp_whole = (event_msg->event_syndrome[0] >> 2) & SYNDROME_TEMP_MASK;

	sprintf(dbg_msg->syndrome, "%d.%d C", temp_whole, temp_fract);
}

static void parse_cm_err_syndrome(struct device_mgmt_event_msg_t *event_msg,
				  struct event_dbg_msg *dbg_msg)
{
	switch (event_msg->event_syndrome[0]) {
	case SP_RECOVERABLE_FW_MM_HANG:
		sprintf(dbg_msg->syndrome,
			"SP Recoverable MM FW RT Hang (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case SP_RECOVERABLE_FW_MM_ERROR:
		sprintf(dbg_msg->syndrome,
			"SP Recoverable MM RT Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_CM_RUNTIME_ERROR:
		sprintf(dbg_msg->syndrome,
			"CM Runtime Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_SPW_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM SPW Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_DISPATCHER_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM Dispatcher Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_SQW_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM SQW Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_SQW_HP_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM SQW HP Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_DMAW_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM DMAW Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_FW_MM_KW_ERROR:
		sprintf(dbg_msg->syndrome,
			"MM KW Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_KERNEL_LAUNCH:
		sprintf(dbg_msg->syndrome,
			"OPS API Kernel Launch Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_DMA_READLIST:
		sprintf(dbg_msg->syndrome,
			"OPS API DMA ReadList Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_DMA_WRITELIST:
		sprintf(dbg_msg->syndrome,
			"OPS API DMA WriteList Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_ECHO:
		sprintf(dbg_msg->syndrome,
			"OPS API Echo Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_FW_VER:
		sprintf(dbg_msg->syndrome,
			"OPS API FW Version Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_COMPATIBILITY:
		sprintf(dbg_msg->syndrome,
			"OPS API API Compatibility Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_ABORT:
		sprintf(dbg_msg->syndrome,
			"OPS API Abort Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_CM_RESET:
		sprintf(dbg_msg->syndrome,
			"OPS API CM Reset Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_TRACE_RT_CONFIG:
		sprintf(dbg_msg->syndrome,
			"OPS API Trace RT Config Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	case MM_RECOVERABLE_OPS_API_TRACE_RT_CONTROL:
		sprintf(dbg_msg->syndrome,
			"OPS API Trace RT Control Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	default:
		sprintf(dbg_msg->syndrome,
			"Undefined Error (error code: %d)\n",
			(s32)event_msg->event_syndrome[1]);
		break;
	}
}

static void parse_throttling_syndrome(struct device_mgmt_event_msg_t *event_msg,
				      struct event_dbg_msg *dbg_msg)
{
	sprintf(dbg_msg->syndrome,
		"Thermal Throttling Duration Beyond Threshold: %u msec\n",
		(u32)GET_OVER_THROTTLE_DURATION_BITS(
			event_msg->event_syndrome[0]));
}

static void parse_sp_runtime_syndrome(struct device_mgmt_event_msg_t *event_msg,
				      struct event_dbg_msg *dbg_msg,
				      struct et_cqueue *cq)
{
	char value_str[VALUE_STR_MAX_LEN];
	u16 idx = 0;
	void *trace_buf;
	struct pci_dev *pdev;
	void __iomem *trace_addr;
	struct et_mapped_region *trace_region;
	u64 trace_buf_size = sizeof(struct trace_execution_stack);
	struct trace_execution_stack *trace_stack;

	trace_region = cq->vq_common->trace_region;
	pdev = cq->vq_common->pdev;

	if (!trace_region->is_valid) {
		dev_err(&pdev->dev, "Invalid Trace Region");
		return;
	}

	if ((u64)(event_msg->event_syndrome[1] + trace_buf_size) >
	    trace_region->size) {
		dev_err(&pdev->dev,
			"Invalid Trace Buffer Offset: 0x%llx\n",
			event_msg->event_syndrome[1]);
		return;
	}

	trace_buf = kmalloc(trace_buf_size + 1, GFP_KERNEL);
	if (IS_ERR(trace_buf)) {
		dev_err(&pdev->dev,
			"Failed to allocate trace_buf , error %ld\n",
			PTR_ERR(trace_buf));
		return;
	}

	trace_stack = (struct trace_execution_stack *)trace_buf;

	/* Get the Device Trace base address */
	trace_addr = (u8 __iomem *)trace_region->mapped_baseaddr;

	/* Read the Device Trace buffer (Base + Offset(from Syndrome) */
	et_ioread(trace_addr,
		  event_msg->event_syndrome[1],
		  (u8 *)trace_stack,
		  trace_buf_size);

	if (trace_stack->header.type != TRACE_TYPE_EXCEPTION) {
		dev_err(&pdev->dev,
			"Invalid type: %d in SP trace entry header\n",
			trace_stack->header.type);
		goto error_free_trace_buf;
	}

	if (strlcat(dbg_msg->syndrome,
		    "Service Processor Error\n",
		    ET_EVENT_SYNDROME_LEN) >= ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	/* Print SP timer ticks count */
	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "Cycles    : 0x%llX\n",
		 trace_stack->header.cycle);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	/* print CSRs (mepc, mstatus, mtval, mcause) */
	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mepc   : 0x%llX\n",
		 trace_stack->registers.epc);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mstatus: 0x%llX\n",
		 trace_stack->registers.status);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mtval  : 0x%llX\n",
		 trace_stack->registers.tval);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mcause : 0x%llX\n",
		 trace_stack->registers.cause);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	/*
	 * Print SP GPRs - GPRs are stored in trace
	 * buffer starting from x1 then x5 to x31
	 */
	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "x1 : 0x%llX\n",
		 trace_stack->registers.gpr[0]);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}
	for (idx = 4; idx < DEV_GPR_REGISTERS; idx++) {
		snprintf(value_str,
			 VALUE_STR_MAX_LEN,
			 "x%-2d: 0x%llX\n",
			 idx + 1,
			 trace_stack->registers.gpr[idx]);
		if (strlcat(dbg_msg->syndrome,
			    value_str,
			    ET_EVENT_SYNDROME_LEN) >= ET_EVENT_SYNDROME_LEN) {
			dev_err(&pdev->dev, "Syndrome string out of space\n");
			goto error_free_trace_buf;
		}
	}

error_free_trace_buf:
	/* free up trace buffer */
	kfree(trace_buf);
}

static void
parse_runtime_error_syndrome(struct device_mgmt_event_msg_t *event_msg,
			     struct event_dbg_msg *dbg_msg)
{
	/* The error count above the threshold is present in the syndrome[1] */
	sprintf(dbg_msg->syndrome,
		"Runtime Error Count Beyond Threshold: %llu \n",
		event_msg->event_syndrome[1]);
}

int et_handle_device_event(struct et_cqueue *cq,
			   struct device_mgmt_event_msg_t *event_msg)
{
	char syndrome_str[ET_EVENT_SYNDROME_LEN];
	struct pci_dev *pdev;
	struct et_pci_dev *et_dev;
	struct event_dbg_msg dbg_msg;
	int rv;

	if (!cq || !event_msg)
		return -EINVAL;

	rv = event_msg->event_info.size;
	pdev = cq->vq_common->pdev;
	et_dev = pci_get_drvdata(pdev);

	switch (event_msg->class_count & EVENT_CLASS_MASK) {
	case ECLASS_INFO:
		dbg_msg.level = LEVEL_INFO;
		break;
	case ECLASS_WARNING:
		dbg_msg.level = LEVEL_WARN;
		break;
	case ECLASS_CRITICAL:
		dbg_msg.level = LEVEL_CRITICAL;
		break;
	case ECLASS_FATAL:
		dbg_msg.level = LEVEL_FATAL;
		break;
	default:
		dev_err(&pdev->dev, "Event class is invalid\n");
		rv = -EINVAL;
		break;
	}

	dbg_msg.count = (event_msg->class_count >> 2) & EVENT_COUNT_MASK;

	syndrome_str[0] = '\0';
	dbg_msg.syndrome = syndrome_str;

	switch (event_msg->event_info.msg_id) {
	case DEV_MGMT_API_MID_PCIE_CE_EVENT:
		dbg_msg.desc = "PCIe Correctable Error";
		parse_pcie_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_PCIE_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_PCIE_UCE_EVENT:
		dbg_msg.desc = "PCIe Un-Correctable Error";
		parse_pcie_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_PCIE_UCE_COUNT]);
		break;
	case DEV_MGMT_API_MID_DRAM_CE_EVENT:
		dbg_msg.desc = "DRAM Correctable Error";
		parse_dram_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_DRAM_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_DRAM_UCE_EVENT:
		dbg_msg.desc = "DRAM Un-Correctable Error";
		parse_dram_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_DRAM_UCE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SRAM_CE_EVENT:
		dbg_msg.desc = "SRAM Correctable Error";
		parse_sram_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_SRAM_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SRAM_UCE_EVENT:
		dbg_msg.desc = "SRAM Un-Correctable Error";
		parse_sram_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_SRAM_UCE_COUNT]);
		break;
	case DEV_MGMT_API_MID_THERMAL_LOW_EVENT:
		dbg_msg.desc = "Temperature Overshoot Warning";
		parse_thermal_syndrome(event_msg, &dbg_msg);
		atomic64_inc(&et_dev->mgmt.err_stats
				      [ET_ERR_STATS_THERM_OVERSHOOT_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_PMIC_ERROR_EVENT:
		dbg_msg.desc = "Power Management IC Errors";
		parse_pmic_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_PMIC_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_MINION_RUNTIME_ERROR_EVENT:
		dbg_msg.desc = "Minion Runtime Error";
		parse_cm_err_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_MINION_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_MINION_RUNTIME_HANG_EVENT:
		dbg_msg.desc = "Minion Runtime Hang";
		parse_cm_err_syndrome(event_msg, &dbg_msg);
		atomic64_inc(
			&et_dev->mgmt
				 .err_stats[ET_ERR_STATS_MINION_HANG_UCE_COUNT]);
		break;
	case DEV_MGMT_API_MID_THROTTLE_TIME_EVENT:
		dbg_msg.desc = "Thermal Throttling Error";
		parse_throttling_syndrome(event_msg, &dbg_msg);
		atomic64_inc(&et_dev->mgmt.err_stats
				      [ET_ERR_STATS_THERM_THROTTLE_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SP_RUNTIME_EXCEPTION_EVENT:
		dbg_msg.desc = "SP Runtime Exception";
		parse_sp_runtime_syndrome(event_msg, &dbg_msg, cq);
		atomic64_inc(
			&et_dev->mgmt
				 .err_stats[ET_ERR_STATS_SP_EXCEPT_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SP_RUNTIME_HANG_EVENT:
		dbg_msg.desc = "SP Runtime Hang";
		parse_sp_runtime_syndrome(event_msg, &dbg_msg, cq);
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_SP_HANG_UCE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SP_RUNTIME_ERROR_EVENT:
		dbg_msg.desc = "SP Runtime Error";
		parse_runtime_error_syndrome(event_msg, &dbg_msg);
		atomic64_inc(&et_dev->mgmt.err_stats[ET_ERR_STATS_SP_CE_COUNT]);
		break;
	case DEV_MGMT_API_MID_SP_WATCHDOG_RESET_EVENT:
		dbg_msg.desc = "SP rebooted due to watchdog timeout";
		atomic64_inc(
			&et_dev->mgmt.err_stats[ET_ERR_STATS_SP_WDOG_UCE_COUNT]);
		break;
		//TODO SW-12608: Add SP stats parsing support in event handler
	case DEV_MGMT_API_MID_SP_OP_STATS_EVENT:
		dbg_msg.desc = "SP Stats event received";
		break;
	default:
		dbg_msg.desc = "Un-Supported Event MSG ID";
		dev_err(&pdev->dev,
			"Event MSG ID [%d] is invalid\n",
			event_msg->event_info.msg_id);
		rv = -EINVAL;
		break;
	}

	et_print_event(pdev, &dbg_msg);

	return rv;
}
