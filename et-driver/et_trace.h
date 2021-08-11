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

/*
 * Max size of string to log into Trace string message.
 *  Longer strings are truncated.
 */
#define TRACE_STRING_MAX_SIZE 64

/*
 * Trace header magic number. This is just temproray number as of now.
 */
#define TRACE_MAGIC_HEADER 0x76543210

/*
 * Trace packet types.
 */
enum trace_type { TRACE_TYPE_STRING, TRACE_TYPE_EXCEPTION = 9 };

/*
 * This Trace packet entry header is for Master Minion.
 */
struct trace_entry_header_mm {
	u64 cycle; /* Current cycle */
	u32 hart_id; /* Hart ID of the Hart which is logging Trace */
	u16 type; /* One of enum trace_type */
	u8 pad[2]; /* Cache Alignment for MM as it uses atomic operations. */
} __packed;

/*
 * This Trace packet entry header is a generic implementation. In general,
 * this should be used until unless specific requirements needs other packer header.
 */
struct trace_entry_header_generic {
	u64 cycle; /* Current cycle */
	u16 type; /* One of enum trace_type */
	u8 pad[6]; /* To keep natural alignment for memory operations. */
} __packed;

/*
 * This structure includes different implementations of headers in a
 * union. So user must know what implementation names it needs to access.
 */
struct trace_entry_header {
	union {
		struct trace_entry_header_generic generic;
		struct trace_entry_header_mm mm;
	};
} __packed;

#endif
