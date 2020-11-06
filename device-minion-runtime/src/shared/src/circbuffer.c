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

// TODO: Do we need this function?
int64_t CIRCBUFFER_push(volatile struct circ_buf_header *restrict const circbuffer_ptr,
                        void *restrict const dest_buffer, const void *restrict const src_buffer,
                        uint32_t length)
{
    uint32_t head_index = circbuffer_ptr->head;
    uint32_t tail_index = circbuffer_ptr->tail;
    uint32_t head_offset = head_index * CIRCBUFFER_SIZE;

    if (CIRCBUFFER_free(head_index, tail_index) == 0) {
        return CIRCBUFFER_ERROR_FULL;
    }

    if (length > CIRCBUFFER_SIZE) {
        return CIRCBUFFER_ERROR_BAD_LENGTH;
    }

    if (head_index >= CIRCBUFFER_COUNT) {
        return CIRCBUFFER_ERROR_BAD_HEAD_INDEX;
    } else {
        // Copy the actual length
        memcpy(((uint8_t *)dest_buffer + head_offset), src_buffer, length);

        circbuffer_ptr->head = (uint16_t)((circbuffer_ptr->head + 1) % CIRCBUFFER_COUNT);

        return (int64_t)length;
    }
}

// TODO: Do we need this function?
int64_t CIRCBUFFER_pop(volatile struct circ_buf_header *restrict const circbuffer_ptr,
                       const void *restrict const src_buffer, void *restrict const dest_buffer,
                       uint32_t length)
{
    uint32_t head_index = circbuffer_ptr->head;
    uint32_t tail_index = circbuffer_ptr->tail;
    uint32_t tail_offset = tail_index * CIRCBUFFER_SIZE;

    if (CIRCBUFFER_used(head_index, tail_index) == 0) {
        return CIRCBUFFER_ERROR_EMPTY;
    }

    if (length > CIRCBUFFER_used(head_index, tail_index)) {
        return CIRCBUFFER_ERROR_BAD_LENGTH;
    }

    if (tail_index >= CIRCBUFFER_COUNT) {
        return CIRCBUFFER_ERROR_BAD_TAIL_INDEX;
    } else {
        if (dest_buffer == NULL) {
            // Throw away length bytes
            circbuffer_ptr->tail = (uint16_t)((circbuffer_ptr->tail + 1) % CIRCBUFFER_COUNT);
            return CIRCBUFFER_ERROR_DATA_DROPPED;
        } else {
            // Copy the actual length of data requested
            memcpy(dest_buffer, ((const uint8_t *const)src_buffer + tail_offset), length);

            circbuffer_ptr->tail = (uint16_t)((circbuffer_ptr->tail + 1) % CIRCBUFFER_COUNT);

            return (int64_t)length;
        }
    }
}
