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

#include <linux/sched.h>

#include "et_io.h"

/**
 * et_iowrite() - Write data from source buffer in IO memory
 * @dst: Pointer to destination address (64-bit aligned) in IO memory
 * @offset: Offset from destination address in IO memory
 * @src: Pointer to source buffer to write data from
 * @count: number of bytes to write
 */
void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count)
{
	size_t qwords;

	// Write until next u16 alignment
	if (offset & 0x1 && count) {
		iowrite8(*src, dst + offset);
		++offset;
		++src;
		--count;
	}

	// Write until next u32 alignment
	if (offset & 0x2 && count >= 0x2) {
		iowrite16(*(u16 *)src, dst + offset);
		offset += 2;
		src += 2;
		count -= 2;
	}

	// Write until next u64 alignment
	if (offset & 0x4 && count >= 0x4) {
		iowrite32(*(u32 *)src, dst + offset);
		offset += 4;
		src += 4;
		count -= 4;
	}

	// Write u64 aligned values
	qwords = count / 8;
	while (qwords) {
		iowrite64(*(u64 *)src, dst + offset);
		offset += 8;
		src += 8;
		count -= 8;
		--qwords;
		cond_resched();
	}

	// Write remaining 4 bytes, if any
	if (count >= 0x4) {
		iowrite32(*(u32 *)src, dst + offset);
		offset += 4;
		src += 4;
		count -= 4;
	}

	// Write remaining 2 bytes, if any
	if (count >= 0x2) {
		iowrite16(*(u16 *)src, dst + offset);
		offset += 2;
		src += 2;
		count -= 2;
	}

	// Write remaining 1 byte, if any
	if (count) {
		iowrite8(*src, dst + offset);
		++offset;
		++src;
		--count;
	}
}

/**
 * et_ioread() - Read data into buffer from source IO memory
 * @src: Pointer to source address (64-bit aligned) in IO memory
 * @offset: Offset from source address in IO memory
 * @dst: Pointer to destination buffer to read data into
 * @count: number of bytes to read
 */
void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count)
{
	size_t qwords;

	// Read until next u16 alignment
	if (offset & 0x1 && count) {
		*dst = ioread8(src + offset);
		++offset;
		++dst;
		--count;
	}

	// Read until next u32 alignment
	if (offset & 0x2 && count >= 0x2) {
		*(u16 *)dst = ioread16(src + offset);
		offset += 2;
		dst += 2;
		count -= 2;
	}

	// Read until next u64 alignment
	if (offset & 0x4 && count >= 0x4) {
		*(u32 *)dst = ioread32(src + offset);
		offset += 4;
		dst += 4;
		count -= 4;
	}

	// Read u64 aligned values
	qwords = count / 8;
	while (qwords) {
		*(u64 *)dst = ioread64(src + offset);
		offset += 8;
		dst += 8;
		count -= 8;
		--qwords;
		cond_resched();
	}

	// Read remaining 4 bytes, if any
	if (count >= 0x4) {
		*(u32 *)dst = ioread32(src + offset);
		offset += 4;
		dst += 4;
		count -= 4;
	}

	// Read remaining 2 bytes, if any
	if (count >= 0x2) {
		*(u16 *)dst = ioread16(src + offset);
		offset += 2;
		dst += 2;
		count -= 2;
	}

	// Read remaining 1 byte, if any
	if (count) {
		*dst = ioread8(src + offset);
		++offset;
		++dst;
		--count;
	}
}
