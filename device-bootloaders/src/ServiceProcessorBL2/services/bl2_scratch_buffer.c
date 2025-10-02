/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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

