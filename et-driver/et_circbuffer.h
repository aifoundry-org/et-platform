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

#ifndef __ET_CIRCBUFFER_H
#define __ET_CIRCBUFFER_H

#include <linux/types.h>
#include <linux/mutex.h>

// TODO: buf should be at 64-bit aligned offset
struct et_circbuffer {
	u32 head;
	u32 tail;
	u32 len;
	u8 __iomem buf[];
} __attribute__ ((__packed__));

u32 et_circbuffer_used(struct et_circbuffer *cb);
u32 et_circbuffer_free(struct et_circbuffer *cb);
bool et_circbuffer_push(struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len);
bool et_circbuffer_peek(struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u32 peek_offset);
bool et_circbuffer_pop(struct et_circbuffer __iomem *cb_mem, u8 *buf,
		       size_t len);

#endif
