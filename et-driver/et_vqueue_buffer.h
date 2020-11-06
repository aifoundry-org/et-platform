// SPDX-License-Identifier: GPL-2.0

/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 *-------------------------------------------------------------------------
 */

#ifndef __ET_VQUEUE_BUFFER_H
#define __ET_VQUEUE_BUFFER_H

#include <linux/types.h>
#include "linux/circ_buf.h"

ssize_t et_vqueue_buffer_write(void __iomem *dst_buf, u32 dst_buf_offset,
			       u16 dst_buf_len, char *src_buf,
			       u16 src_buf_len);
ssize_t et_vqueue_buffer_read(void __iomem *src_buf, u32 src_buf_offset,
			      u16 src_buf_len, char *dst_buf,
			      u16 dst_buf_len);

#endif
