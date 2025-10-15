/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

#include <stdint.h>
#include "bl2_scratch_buffer.h"

#define GLOBAL_SCRATCH_SIZE    72      // unit in KB, needs to be at least 64KB for DDR FW

static uint8_t g_scratch_buffer[GLOBAL_SCRATCH_SIZE*1024] __attribute__ ((aligned(64)));

void *get_scratch_buffer(uint32_t *size)
{
    *size = GLOBAL_SCRATCH_SIZE*1024;
    return g_scratch_buffer;
}

