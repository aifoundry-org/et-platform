// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#include "et_vqueue_buffer.h"
#include "et_io.h"

ssize_t et_vqueue_buffer_write(void __iomem *dst_buf, u32 dst_buf_offset,
			       u16 dst_buf_len, char *src_buf, u16 src_buf_len)
{
	if (dst_buf_offset + src_buf_len > dst_buf_len) {
		pr_err("Not enough space in destination buffer\n");
		return -ENOMEM;
	}

	et_iowrite(dst_buf, dst_buf_offset, src_buf, src_buf_len);

	return dst_buf_offset + src_buf_len;
}

ssize_t et_vqueue_buffer_read(void __iomem *src_buf, u32 src_buf_offset,
			      u16 src_buf_len, char *dst_buf, u16 dst_buf_len)
{
	if ((src_buf_len - src_buf_offset) < dst_buf_len) {
		pr_err("Read requested more than available data\n");
		return -ENOMEM;
	}

	et_ioread(src_buf, src_buf_offset, dst_buf, dst_buf_len);

	return src_buf_offset + dst_buf_len;
}
