/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "circbuffer.h"
#include "log.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void CIRCBUFFER_init(volatile struct circ_buf_header *const circbuffer_ptr, void *buffer_ptr,
                     uint64_t buffer_length)
{
    circbuffer_ptr->head = 0;
    circbuffer_ptr->tail = 0;

    memset(buffer_ptr, 0, buffer_length);
}

int64_t CIRCBUFFER_write(void *restrict const dest_buffer, uint64_t dest_buffer_offset,
                         uint64_t dest_buffer_length, const void *restrict const src_buffer,
                         uint64_t src_buffer_length)
{
    uint8_t *const dest_ptr = dest_buffer;
    const uint8_t *const src_ptr = src_buffer;

    if ((dest_buffer_offset + src_buffer_length) > dest_buffer_length) {
        log_write(LOG_LEVEL_ERROR, "Bad Length! Required: %" PRIu64 " Available: %" PRIu64 "\r\n",
                  (dest_buffer_offset + src_buffer_length), dest_buffer_length);
        return CIRCBUFFER_ERROR_BAD_LENGTH;
    }

    for (uint64_t i = 0; i < src_buffer_length; i++) {
        dest_ptr[dest_buffer_offset++] = src_ptr[i];
    }

    return (int64_t)src_buffer_length;
}

int64_t CIRCBUFFER_read(void *restrict const src_buffer, uint64_t src_buffer_offset,
                        uint64_t src_buffer_length, void *restrict const dest_buffer,
                        uint64_t dest_buffer_length)
{
    uint8_t *const dest_ptr = dest_buffer;
    uint8_t *const src_ptr = src_buffer;

    if ((src_buffer_length - src_buffer_offset) < dest_buffer_length) {
        return CIRCBUFFER_ERROR_BAD_LENGTH;
    }

    for (uint64_t i = 0; i < dest_buffer_length; i++) {
        dest_ptr[i] = src_ptr[src_buffer_offset++];
    }

    return (int64_t)dest_buffer_length;
}
