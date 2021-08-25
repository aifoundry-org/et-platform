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
	/* Release 0.9.0 */
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
	case CM_USER_KERNEL_ERROR:
		strcat(dbg_msg->syndrome, "CM User Kernel Error\n");
		break;
	case CM_RUNTIME_ERROR:
		strcat(dbg_msg->syndrome, "CM Runtime Error\n");
		break;
	case MM_DISPATCHER_ERROR:
		strcat(dbg_msg->syndrome, "MM Dispatcher Error\n");
		break;
	case MM_SQW_ERROR:
		strcat(dbg_msg->syndrome, "MM SQW Error\n");
		break;
	case MM_DMW_ERROR:
		strcat(dbg_msg->syndrome, "MM DMW Error\n");
		break;
	case MM_KW_ERROR:
		strcat(dbg_msg->syndrome, "MM KW Error\n");
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
	u8 *trace_data;
	struct pci_dev *pdev;
	void __iomem *trace_addr;
	struct et_mapped_region *trace_region;
	u64 trace_buf_size = sizeof(struct trace_entry_header) +
			     SP_EXCEPTION_FRAME_SIZE + SP_GLOBALS_SIZE;
	struct trace_entry_header *entry_header;

	trace_region = cq->vq_common->trace_region;
	pdev = cq->vq_common->pdev;

	if (!trace_region->is_valid) {
		dev_err(&pdev->dev, "Invalid Trace Region");
		return;
	}

	if ((u64)(event_msg->event_syndrome[1] + trace_buf_size) >
	    trace_region->size) {
		dev_err(&pdev->dev, "Invalid Trace Buffer Offset");
		return;
	}

	trace_buf = kmalloc(trace_buf_size + 1, GFP_KERNEL);
	if (IS_ERR(trace_buf)) {
		dev_err(&pdev->dev,
			"Failed to allocate trace_buf , error %ld\n",
			PTR_ERR(trace_buf));
		return;
	}

	trace_data = (u8 *)trace_buf;
	trace_data[trace_buf_size] = '\0';

	/* Get the Device Trace base address */
	trace_addr = (u8 __iomem *)trace_region->mapped_baseaddr;

	/* Read the Device Trace buffer (Base + Offset(from Syndrome) */
	et_ioread(trace_addr,
		  event_msg->event_syndrome[1],
		  (u8 *)trace_data,
		  trace_buf_size);

	entry_header = (struct trace_entry_header *)trace_data;

	if (entry_header->type != TRACE_TYPE_EXCEPTION) {
		dev_err(&pdev->dev,
			"Invalid type: %d in SP trace entry header\n",
			entry_header->type);
		goto error_free_trace_buf;
	}

	if (strlcat(dbg_msg->syndrome,
		    "Service Processor Error\n",
		    ET_EVENT_SYNDROME_LEN) >= ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	/*
	 * Print SP timer ticks count and GPRs - GPRs are stored in
	 * trace buffer starting from x1 then x5 to x31
	 */
	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "Cycles    : 0x%llX\n",
		 entry_header->cycle);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	trace_data += sizeof(struct trace_entry_header);

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "x1 : 0x%llX\n",
		 *(u64 *)trace_data++);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}

	for (idx = 1; idx < SP_GPR_REGISTERS; idx++) {
		snprintf(value_str,
			 VALUE_STR_MAX_LEN,
			 "x%-2d: 0x%llX\n",
			 idx + 4,
			 *(u64 *)trace_data++);
		if (strlcat(dbg_msg->syndrome,
			    value_str,
			    ET_EVENT_SYNDROME_LEN) >= ET_EVENT_SYNDROME_LEN) {
			dev_err(&pdev->dev, "Syndrome string out of space\n");
			goto error_free_trace_buf;
		}
		if (trace_data >= (u8 *)trace_buf + trace_buf_size) {
			dev_err(&pdev->dev,
				"Reached end of SP trace buffer, exiting syndrome parsing!\n");
			goto error_free_trace_buf;
		}
	}

	/* print CSRs (mepc, mstatus, mtval, mcause) */
	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mepc   : 0x%llX\n",
		 *(u64 *)trace_data++);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}
	if (trace_data >= (u8 *)trace_buf + trace_buf_size) {
		dev_err(&pdev->dev,
			"Reached end of SP trace buffer, exiting syndrome parsing!\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mstatus: 0x%llX\n",
		 *(u64 *)trace_data++);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}
	if (trace_data >= (u8 *)trace_buf + trace_buf_size) {
		dev_err(&pdev->dev,
			"Reached end of SP trace buffer, exiting syndrome parsing!\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mtval  : 0x%llX\n",
		 *(u64 *)trace_data++);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
	}
	if (trace_data >= (u8 *)trace_buf + trace_buf_size) {
		dev_err(&pdev->dev,
			"Reached end of SP trace buffer, exiting syndrome parsing!\n");
		goto error_free_trace_buf;
	}

	snprintf(value_str,
		 VALUE_STR_MAX_LEN,
		 "mcause : 0x%llX\n",
		 *(u64 *)trace_data);
	if (strlcat(dbg_msg->syndrome, value_str, ET_EVENT_SYNDROME_LEN) >=
	    ET_EVENT_SYNDROME_LEN) {
		dev_err(&pdev->dev, "Syndrome string out of space\n");
		goto error_free_trace_buf;
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
	struct event_dbg_msg dbg_msg;
	int rv;

	if (!cq || !event_msg)
		return -EINVAL;

	rv = event_msg->event_info.size;
	pdev = cq->vq_common->pdev;

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
		break;
	case DEV_MGMT_API_MID_PCIE_UCE_EVENT:
		dbg_msg.desc = "PCIe Un-Correctable Error";
		parse_pcie_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_DRAM_CE_EVENT:
		dbg_msg.desc = "DRAM Correctable Error";
		parse_dram_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_DRAM_UCE_EVENT:
		dbg_msg.desc = "DRAM Un-Correctable Error";
		parse_dram_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_SRAM_CE_EVENT:
		dbg_msg.desc = "SRAM Correctable Error";
		parse_sram_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_SRAM_UCE_EVENT:
		dbg_msg.desc = "SRAM Un-Correctable Error";
		parse_sram_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_THERMAL_LOW_EVENT:
		dbg_msg.desc = "Temperature Overshoot Warning";
		parse_thermal_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_PMIC_ERROR_EVENT:
		dbg_msg.desc = "Power Management IC Errors";
		parse_pmic_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_CM_ETH_EVENT:
		dbg_msg.desc = "Compute Minion Exception";
		parse_cm_err_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_CM_HTH_EVENT:
		dbg_msg.desc = "Compute Minion Hang";
		parse_cm_err_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_THROTTLE_TIME_EVENT:
		dbg_msg.desc = "Thermal Throttling Error";
		parse_throttling_syndrome(event_msg, &dbg_msg);
		break;
	case DEV_MGMT_API_MID_SP_RUNTIME_EXCEPTION_EVENT:
	case DEV_MGMT_API_MID_SP_RUNTIME_HANG_EVENT:
		dbg_msg.desc = "SP Runtime Exception";
		parse_sp_runtime_syndrome(event_msg, &dbg_msg, cq);
		break;
	case DEV_MGMT_API_MID_SP_RUNTIME_ERROR_EVENT:
		dbg_msg.desc = "SP Runtime Error";
		parse_runtime_error_syndrome(event_msg, &dbg_msg);
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
