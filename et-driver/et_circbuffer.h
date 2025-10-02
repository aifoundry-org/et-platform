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

#include <linux/mutex.h>
#include <linux/types.h>

#define ET_CB_SYNC_FOR_HOST   BIT(0)
#define ET_CB_SYNC_FOR_DEVICE BIT(1)

/**
 * struct et_circbuffer - The circular buffer control block
 * @head: Offset of the circular buffer to write data to
 * @tail: Offset of the circular buffer to read data from
 * @len: Total length (in bytes) of the circular buffer
 * @pad: Padding for alignment
 * @buf: Flexible array to access circular buffer memory
 */
struct et_circbuffer {
	u64 head;
	u64 tail;
	u64 len;
	u64 pad;
	u8 __iomem buf[];
} __packed __aligned(8);

/**
 * et_circbuffer_used() - Get number of used bytes in circular buffer
 * @cb: pointer to et_circbuffer structure
 *
 * Return: Used space in bytes
 */
static inline u64 et_circbuffer_used(struct et_circbuffer *cb)
{
	if (cb->head >= cb->tail)
		return cb->head - cb->tail;
	return cb->len + cb->head - cb->tail;
}

/**
 * et_circbuffer_free() - Get number of free bytes in circular buffer
 * @cb: pointer to et_circbuffer structure
 *
 * Return: Free space in bytes
 */
static inline u64 et_circbuffer_free(struct et_circbuffer *cb)
{
	if (cb->head >= cb->tail)
		return (cb->len - 1U) - (cb->head - cb->tail);
	return cb->tail - cb->head - 1U;
}

bool et_circbuffer_push(struct et_circbuffer *cb,
			struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u8 sync);
bool et_circbuffer_pop(struct et_circbuffer *cb,
		       struct et_circbuffer __iomem *cb_mem, u8 *buf,
		       size_t len, u8 sync);

#endif
