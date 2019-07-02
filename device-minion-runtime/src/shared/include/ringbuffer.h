#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define RINGBUFFER_LENGTH 2032

static_assert(RINGBUFFER_LENGTH % 8 == 0, "ringbuffer length must be 8-byte aligned");

#define RINGBUFFER_MAX_LENGTH (RINGBUFFER_LENGTH - 1U)

#define RINGBUFFER_ERROR_BAD_LENGTH -1
#define RINGBUFFER_ERROR_DATA_DROPPED -2

typedef struct ringbuffer_s
{
    uint32_t head_index;
    uint32_t tail_index;
    uint8_t queue[RINGBUFFER_LENGTH];
} ringbuffer_t __attribute__ ((aligned(8)));

void RINGBUFFER_init(volatile ringbuffer_t* const ringbuffer_ptr);
int64_t RINGBUFFER_write(volatile ringbuffer_t* restrict const ringbuffer_ptr, const void* restrict const data_ptr, uint32_t length);
int64_t RINGBUFFER_read(volatile ringbuffer_t* restrict const ringbuffer_ptr, void* restrict const data_ptr, uint32_t length);
uint64_t RINGBUFFER_free(volatile const ringbuffer_t* const ringbuffer_ptr);
uint64_t RINGBUFFER_used(volatile const ringbuffer_t* const ringbuffer_ptr);
bool RINGBUFFER_empty(volatile const ringbuffer_t* const ringbuffer_ptr);
bool RINGBUFFER_full(volatile const ringbuffer_t* const ringbuffer_ptr);

#endif
