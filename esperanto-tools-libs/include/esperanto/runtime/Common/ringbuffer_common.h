#ifndef RINGBUFFER_COMMON_H
#define RINGBUFFER_COMMON_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define RINGBUFFER_LENGTH 496

static_assert(RINGBUFFER_LENGTH % 8 == 0, "ringbuffer length must be 8-byte aligned");

#define RINGBUFFER_MAX_LENGTH (RINGBUFFER_LENGTH - 1U)

#define RINGBUFFER_ERROR_BAD_LENGTH     -1
#define RINGBUFFER_ERROR_DATA_DROPPED   -2
#define RINGBUFFER_ERROR_BAD_HEAD_INDEX -3
#define RINGBUFFER_ERROR_BAD_TAIL_INDEX -4

typedef struct ringbuffer_s {
    uint32_t head_index;
    uint32_t tail_index;
    uint8_t queue[RINGBUFFER_LENGTH];
} ringbuffer_t __attribute__((aligned(8)));

#endif
