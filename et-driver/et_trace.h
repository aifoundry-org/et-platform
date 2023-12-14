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

#ifndef DEVICE_TRACE_TYPES_H
#define DEVICE_TRACE_TYPES_H

/* Number of GPR registers for execution stack context */
#define DEV_GPR_REGISTERS 31

/**
 * enum trace_type - Trace packet types
 */
enum trace_type { TRACE_TYPE_EXCEPTION = 12 };

/*
 * struct trace_entry_header - Trace entry header
 * @cycle: Current cycle
 * @payload_size: Size of event payload following the entry header
 * @hart_id: Hart ID of the Hart which is logging this trace packet
 * @type: Value of enum trace_type
 *
 * This Trace packet entry header is a generic implementation. In general, this
 * should be used until unless specific requirements needs other packer header.
 */
struct trace_entry_header {
	u64 cycle;
	u32 payload_size;
	u16 hart_id;
	u16 type;
} __packed __aligned(8);

/*
 * struct dev_context_registers - Device context registers
 * @epc: epc register
 * @tval: tval register
 * @status: status register
 * @cause: cause register
 * @gpr: Array of gpr registers
 *
 * These are internal values populated when trace_execution_stack is received.
 */
struct dev_context_registers {
	u64 epc;
	u64 tval;
	u64 status;
	u64 cause;
	u64 gpr[DEV_GPR_REGISTERS]; /* x1 to x31 */
} __packed __aligned(8);

/*
 * struct trace_execution_stack - Execution stack of device for exceptions
 * @header: Trace entry header of struct trace_entry_header type
 * @registers: Device context registers of struct dev_context_registers
 */
struct trace_execution_stack {
	struct trace_entry_header header;
	struct dev_context_registers registers;
} __packed __aligned(8);

#endif
