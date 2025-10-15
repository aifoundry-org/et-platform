/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (c) 2025 Ainekko, Co.
 *
 **********************************************************************/

#include <linux/io.h>

/* For performace prefer atomic over non-atomic */
#ifndef ioread64
#ifdef readq
#define ioread64 readq
#endif
#endif

#ifndef iowrite64
#ifdef writeq
#define iowrite64 writeq
#endif
#endif

#include <linux/io-64-nonatomic-lo-hi.h>

void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count);
void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count);
