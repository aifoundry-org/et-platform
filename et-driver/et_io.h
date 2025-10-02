/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (C) 2023 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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
