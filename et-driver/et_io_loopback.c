// SPDX-License-Identifier: GPL-2.0

/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 **********************************************************************/

#include "et_io.h"

void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count)
{
	memcpy((void __force *)dst + offset, src, count);
}

void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count)
{
	memcpy(dst, (void __force *)src + offset, count);
}
