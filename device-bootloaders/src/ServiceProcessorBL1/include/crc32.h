/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>
#include <stdlib.h>

void crc32(const void *data, size_t n_bytes, uint32_t *crc);

#endif
