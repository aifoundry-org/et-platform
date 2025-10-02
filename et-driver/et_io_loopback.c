// SPDX-License-Identifier: GPL-2.0

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

#include "et_io.h"

void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count)
{
	memcpy((void __force *)dst + offset, src, count);
}

void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count)
{
	memcpy(dst, (void __force *)src + offset, count);
}
