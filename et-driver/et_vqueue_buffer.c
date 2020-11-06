// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include "et_vqueue_buffer.h"

#include <linux/io.h>

ssize_t et_vqueue_buffer_write(void __iomem *dst_buf, u32 dst_buf_offset,
			       u16 dst_buf_len, char *src_buf, u16 src_buf_len)
{
	int dwords;

	if (dst_buf_offset + src_buf_len > dst_buf_len) {
		pr_err("Not enough space in destination buffer\n");
		return -ENOMEM;
	}

	//Write until next u32 alignment
	while (dst_buf_offset & 0x3 && src_buf_len) {
		iowrite8(*src_buf, dst_buf + dst_buf_offset);
		++dst_buf_offset;
		++src_buf;
		--src_buf_len;
	}

	//Write u32 aligned values
	dwords = src_buf_len / 4;

	while (dwords) {
		iowrite32(*(u32 *)src_buf, dst_buf + dst_buf_offset);
		dst_buf_offset += 4;
		src_buf += 4;
		src_buf_len -= 4;
		--dwords;
	}

	//Write any remaining bytes (0-3 bytes)
	while (src_buf_len) {
		iowrite8(*src_buf, dst_buf + dst_buf_offset);
		++dst_buf_offset;
		++src_buf;
		--src_buf_len;
	}

	return dst_buf_offset;
}

ssize_t et_vqueue_buffer_read(void __iomem *src_buf, u32 src_buf_offset,
			      u16 src_buf_len, char *dst_buf, u16 dst_buf_len)
{
	int dwords;

	if ((src_buf_len - src_buf_offset) < dst_buf_len) {
		pr_err("Read requested more than available data\n");
		return -ENOMEM;
	}

	//Read until next u32 alignment
	while ((src_buf_offset & 0x3) && dst_buf_len) {
		*dst_buf = ioread8(src_buf + src_buf_offset);
		++src_buf_offset;
		++dst_buf;
		--dst_buf_len;
	}

	//Read u32 aligned values
	dwords = dst_buf_len / 4;

	while (dwords) {
		*(u32 *)dst_buf = ioread32(src_buf + src_buf_offset);
		src_buf_offset += 4;
		dst_buf += 4;
		dst_buf_len -= 4;
		--dwords;
	}

	//Read any remaining bytes (0-3 bytes)
	while (dst_buf_len) {
		*dst_buf = ioread8(src_buf + src_buf_offset);
		++src_buf_offset;
		++dst_buf;
		--dst_buf_len;
	}

	return src_buf_offset;
}
