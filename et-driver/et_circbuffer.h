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

#define ET_CB_SYNC_FOR_HOST	BIT(0)
#define ET_CB_SYNC_FOR_DEVICE	BIT(1)

// TODO: buf should be at 64-bit aligned offset
struct et_circbuffer {
	u32 head;
	u32 tail;
	u32 len;
	u8 __iomem buf[];
} __attribute__ ((__packed__));

static inline u32 et_circbuffer_used(struct et_circbuffer *cb)
{
	if (cb->head >= cb->tail)
		return cb->head - cb->tail;
	return cb->len + cb->head - cb->tail;
}

static inline u32 et_circbuffer_free(struct et_circbuffer *cb)
{
	if (cb->head >= cb->tail)
		return (cb->len - 1U) - (cb->head - cb->tail);
	return cb->tail - cb->head - 1U;
}

/*
 * To minimize IO operations between the successive pushes/pops, the caller can
 * decide to whether use local copy of `cb` or directly read from device memory
 * `cb_mem`. Directly reading from device memory will also update the local
 * copy passed. So, in successive operations, caller should first read from
 * device memory and then can use cached copy.
 */
bool et_circbuffer_push(struct et_circbuffer *cb,
			struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u8 sync);
bool et_circbuffer_pop(struct et_circbuffer *cb,
		       struct et_circbuffer __iomem *cb_mem, u8 *buf,
		       size_t len, u8 sync);

#endif
