/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_RING_BUFFER_H
#define ET_RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

// Final Ring buffer length used for event logging
#define DEVICE_MRT_BUFFER_LENGTH(x) (x - sizeof(struct buffer_header_t))

// Returns buffer address associated with the given hart
#define DEVICE_MRT_BUFFER_BASE(hart_id, size)                               \
    ((void *)ALIGN(DEVICE_MRT_TRACE_BASE + sizeof(struct trace_control_t) + \
                       size * hart_id,                                      \
                   TRACE_BUFFER_REGION_ALIGNEMNT))

void *ring_buffer_alloc_space(uint16_t hartid, size_t size);

#endif  // ET_RING_BUFFER_H
