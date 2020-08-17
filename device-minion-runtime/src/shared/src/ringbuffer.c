#include "ringbuffer.h"

#include <stddef.h>
#include <stdint.h>

void RINGBUFFER_init(volatile ringbuffer_t *const ringbuffer_ptr)
{
    ringbuffer_ptr->head_index = 0;
    ringbuffer_ptr->tail_index = 0;

    for (uint64_t i = 0; i < RINGBUFFER_LENGTH; i++) {
        ringbuffer_ptr->queue[i] = 0;
    }
}

int64_t RINGBUFFER_write(volatile ringbuffer_t *restrict const ringbuffer_ptr,
                         const void *restrict const buffer, uint32_t length)
{
    uint32_t head_index = ringbuffer_ptr->head_index;
    const uint8_t *const data_ptr = buffer;

    if (length > RINGBUFFER_free(ringbuffer_ptr)) {
        return RINGBUFFER_ERROR_BAD_LENGTH;
    }

    if (head_index >= RINGBUFFER_LENGTH) {
        return RINGBUFFER_ERROR_BAD_HEAD_INDEX;
    } else {
        for (uint16_t i = 0; i < length; i++) {
            ringbuffer_ptr->queue[head_index] = data_ptr[i];
            head_index = (head_index + 1U) % RINGBUFFER_LENGTH;
        }

        ringbuffer_ptr->head_index = head_index;

        return (int64_t)length;
    }
}

int64_t RINGBUFFER_read(volatile ringbuffer_t *restrict const ringbuffer_ptr,
                        void *restrict const buffer, uint32_t length)
{
    uint32_t tail_index = ringbuffer_ptr->tail_index;
    uint8_t *const data_ptr = buffer;

    if (length > RINGBUFFER_used(ringbuffer_ptr)) {
        return RINGBUFFER_ERROR_BAD_LENGTH;
    }

    if (tail_index >= RINGBUFFER_LENGTH) {
        return RINGBUFFER_ERROR_BAD_TAIL_INDEX;
    } else {
        if (data_ptr == NULL) {
            // Throw away length bytes
            ringbuffer_ptr->tail_index = (tail_index + length) % RINGBUFFER_LENGTH;
            return RINGBUFFER_ERROR_DATA_DROPPED;
        } else {
            // Read length bytes to data_ptr
            for (uint16_t i = 0; i < length; i++) {
                data_ptr[i] = ringbuffer_ptr->queue[tail_index];
                tail_index = (tail_index + 1U) % RINGBUFFER_LENGTH;
            }

            ringbuffer_ptr->tail_index = tail_index;

            return (int64_t)length;
        }
    }
}

uint64_t RINGBUFFER_free(volatile const ringbuffer_t *const ringbuffer_ptr)
{
    const uint32_t head_index = ringbuffer_ptr->head_index;
    const uint32_t tail_index = ringbuffer_ptr->tail_index;

    return (head_index >= tail_index) ? (RINGBUFFER_LENGTH - 1U) - (head_index - tail_index) :
                                        tail_index - head_index - 1U;
}

uint64_t RINGBUFFER_used(volatile const ringbuffer_t *const ringbuffer_ptr)
{
    const uint32_t head_index = ringbuffer_ptr->head_index;
    const uint32_t tail_index = ringbuffer_ptr->tail_index;

    return (head_index >= tail_index) ? head_index - tail_index :
                                        (RINGBUFFER_LENGTH + head_index - tail_index);
}

bool RINGBUFFER_empty(volatile const ringbuffer_t *const ringbuffer_ptr)
{
    return (ringbuffer_ptr->head_index == ringbuffer_ptr->tail_index);
}

bool RINGBUFFER_full(volatile const ringbuffer_t *const ringbuffer_ptr)
{
    return (((ringbuffer_ptr->head_index + 1U) % RINGBUFFER_LENGTH) == ringbuffer_ptr->tail_index);
}
