// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 **********************************************************************/

#include "et_circbuffer.h"
#include "et_io.h"

bool et_circbuffer_push(struct et_circbuffer *cb,
			struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u8 sync)
{
	size_t bytes_to_write;

	if (!cb || !cb_mem)
		return false;

	if (sync & ET_CB_SYNC_FOR_HOST)
		et_ioread(cb_mem, offsetof(struct et_circbuffer, tail),
			  (u8 *)&cb->tail, sizeof(cb->tail));

	if (len > cb->len) {
		pr_err("message too big (len %ld, max %lld)", len, cb->len);
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
		et_iowrite(cb_mem, offsetof(struct et_circbuffer, head),
			   (u8 *)&cb->head, sizeof(cb->head));

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
		et_ioread(cb_mem, offsetof(struct et_circbuffer, head),
			  (u8 *)&cb->head, sizeof(cb->head));

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
		et_iowrite(cb_mem, offsetof(struct et_circbuffer, tail),
			   (u8 *)&cb->tail, sizeof(cb->tail));

	return true;
}
