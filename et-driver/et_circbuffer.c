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

#include "et_io.h"
#include "et_circbuffer.h"

bool et_circbuffer_push(struct et_circbuffer *cb,
			struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u8 sync)
{
	size_t bytes_to_write;

	if (!cb || !cb_mem)
		return false;

	if (sync & ET_CB_SYNC_FOR_HOST)
		cb->tail = ioread32(&cb_mem->tail);
		// TODO SW-6388: Sync whole circbuffer instead when
		// requested
		// et_ioread(cb_mem, 0, (u8 *)cb, sizeof(*cb));

	if (len > cb->len) {
		pr_err("message too big (len %ld, max %d)", len, cb->len);
		return false;
	}

	if (et_circbuffer_free(cb) < len)
		return false;

	while (len) {
		if ((cb->head + len) > cb->len)
			bytes_to_write = cb->len - cb->head;
		else
			bytes_to_write = len;

		et_iowrite(cb_mem->buf, cb->head, buf, bytes_to_write);

		cb->head = (cb->head + bytes_to_write) % cb->len;
		len -= bytes_to_write;
		buf += bytes_to_write;
	}

	if (sync & ET_CB_SYNC_FOR_DEVICE)
		iowrite32(cb->head, &cb_mem->head);

	return true;
}

bool et_circbuffer_pop(struct et_circbuffer *cb,
		       struct et_circbuffer __iomem *cb_mem, u8 *buf,
		       size_t len, u8 sync)
{
	size_t bytes_to_read;

	if (!cb || !cb_mem)
		return false;

	if (sync & ET_CB_SYNC_FOR_HOST)
		cb->head = ioread32(&cb_mem->head);
		// TODO SW-6388: Sync whole circbuffer instead when
		// requested
		// et_ioread(cb_mem, 0, (u8 *)cb, sizeof(*cb));

	if (et_circbuffer_used(cb) < len)
		return false;

	while (len) {
		if ((cb->tail + len) > cb->len)
			bytes_to_read = cb->len - cb->tail;
		else
			bytes_to_read = len;

		et_ioread(cb_mem->buf, cb->tail, buf, bytes_to_read);

		cb->tail = (cb->tail + bytes_to_read) % cb->len;
		len -= bytes_to_read;
		buf += bytes_to_read;
	}

	if (sync & ET_CB_SYNC_FOR_DEVICE)
		iowrite32(cb->tail, &cb_mem->tail);

	return true;
}
