#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "ringbuffer_common.h"

void RINGBUFFER_init(volatile ringbuffer_t* const ringbuffer_ptr);
int64_t RINGBUFFER_write(volatile ringbuffer_t* restrict const ringbuffer_ptr, const void* restrict const data_ptr, uint32_t length);
int64_t RINGBUFFER_read(volatile ringbuffer_t* restrict const ringbuffer_ptr, void* restrict const data_ptr, uint32_t length);
uint64_t RINGBUFFER_free(volatile const ringbuffer_t* const ringbuffer_ptr);
uint64_t RINGBUFFER_used(volatile const ringbuffer_t* const ringbuffer_ptr);
bool RINGBUFFER_empty(volatile const ringbuffer_t* const ringbuffer_ptr);
bool RINGBUFFER_full(volatile const ringbuffer_t* const ringbuffer_ptr);

#endif
