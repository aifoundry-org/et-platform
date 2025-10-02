// SPDX-License-Identifier: GPL-2.0

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

#include "et_circbuffer.h"
#include "et_io.h"

/**
 * et_circbuffer_push() - Pushes the data from buffer to IO memory circbuffer
 * @cb: et_circbuffer structure pointer to local copy of circular buffer
 * @cb_mem: et_circbuffer structure pointer to IO memory circular buffer
 * @buf: Pointer to source data to be pushed on IO memory circular buffer
 * @len: The length of the source data in bytes
 * @sync: The flag to indicate sync between local copy and IO memory
 *
 * To minimize IO operations between the successive pushes, the caller can
 * decide to whether use local copy `cb` or directly read from device memory
 * `cb_mem`. Directly reading from device memory will also update the local
 * copy passed. So, in successive operations, caller should first read from
 * device memory and then can use cached copy.
 *
 * Return: true on success, false otherwise.
 */
bool et_circbuffer_push(struct et_circbuffer *cb,
			struct et_circbuffer __iomem *cb_mem, u8 *buf,
			size_t len, u8 sync)
{
	size_t bytes_to_write;

	if (!cb || !cb_mem)
		return false;

	if (sync & ET_CB_SYNC_FOR_HOST)
		cb->tail = ioread64(&cb_mem->tail);

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
		iowrite64(cb->head, &cb_mem->head);

	return true;
}

/**
 * et_circbuffer_pop() - Pop out the data into buffer from IO memory circbuffer
 * @cb: et_circbuffer structure pointer to local copy of circular buffer
 * @cb_mem: et_circbuffer structure pointer to IO memory circular buffer
 * @buf: Pointer to buffer to read into from IO memory circular buffer
 * @len: The length of the data to read in bytes
 * @sync: The flag to indicate sync between local copy and IO memory
 *
 * To minimize IO operations between the successive pops, the caller can decide
 * to whether use local copy `cb` or directly read from device memory `cb_mem`.
 * Directly reading from device memory will also update the local copy passed.
 * So, in successive operations, caller should first read from device memory
 * and then can use cached copy.
 *
 * Return: true on success, false otherwise.
 */
bool et_circbuffer_pop(struct et_circbuffer *cb,
		       struct et_circbuffer __iomem *cb_mem, u8 *buf,
		       size_t len, u8 sync)
{
	size_t bytes_to_read;

	if (!cb || !cb_mem)
		return false;

	if (sync & ET_CB_SYNC_FOR_HOST)
		cb->head = ioread64(&cb_mem->head);

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
		iowrite64(cb->tail, &cb_mem->tail);

	return true;
}
