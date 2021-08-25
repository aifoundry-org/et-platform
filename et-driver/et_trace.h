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
 * Trace packet types.
 */
enum trace_type { TRACE_TYPE_EXCEPTION = 9 };

/*
 * This Trace packet entry header is a generic implementation. In general,
 * this should be used until unless specific requirements needs other packer header.
 */
struct trace_entry_header {
	u64 cycle; /* Current cycle */
	u32 hart_id; /* Hart ID of the Hart which is logging Trace */
	u16 type; /* One of enum trace_type */
	u8 pad[2]; /* To keep natural alignment for memory operations. */
} __packed;

#endif
