/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef CIRCBUFFER_COMMON_H
#define CIRCBUFFER_COMMON_H

#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Count and size must be in power of 2
// TODO: Temporarily reduced the count of buffers
#define CIRCBUFFER_COUNT 2
#define CIRCBUFFER_SIZE  128

static_assert((CIRCBUFFER_COUNT & (CIRCBUFFER_COUNT - 1)) == 0,
              "circular buffer count must be power of 2");
static_assert((CIRCBUFFER_SIZE & (CIRCBUFFER_SIZE - 1)) == 0,
              "circular buffer size must be power of 2");

// Total length in bytes
#define CIRCBUFFER_BYTES (CIRCBUFFER_COUNT * CIRCBUFFER_SIZE)

#define CIRCBUFFER_MAX_LENGTH (CIRCBUFFER_COUNT - 1U)

#define CIRCBUFFER_ERROR_BAD_LENGTH     -1
#define CIRCBUFFER_ERROR_DATA_DROPPED   -2
#define CIRCBUFFER_ERROR_BAD_HEAD_INDEX -3
#define CIRCBUFFER_ERROR_BAD_TAIL_INDEX -4
#define CIRCBUFFER_ERROR_FULL           -5
#define CIRCBUFFER_ERROR_EMPTY          -6
// Indicates the end of error codes for circular buffer
#define CIRCBUFFER_ERROR_END -7

/// \brief The circular buffer header which holds the pointers to
/// head and tail.
struct circ_buf_header {
    uint16_t head;
    uint16_t tail;
} __attribute__((__packed__));

#endif
