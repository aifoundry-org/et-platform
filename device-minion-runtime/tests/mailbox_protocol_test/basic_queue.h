#ifndef _BASIC_QUEUE_H_
#define _BASIC_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#define BASIC_QUEUE_INFINITE_DELAY 0xFFFFFFFF

typedef struct BASIC_QUEUE_s {
    size_t element_size;
    size_t elements_count;
    size_t head_index;
    size_t tail_index;
    uint8_t * queue_data;

    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_is_not_full_cond;
    pthread_cond_t queue_is_not_empty_cond;
} BASIC_QUEUE_t;

int basic_queue_create(BASIC_QUEUE_t * queue, size_t element_size, size_t elements_count);
int basic_queue_delete(BASIC_QUEUE_t * queue);
int basic_queue_is_empty(BASIC_QUEUE_t * queue, bool * empty);
int basic_queue_is_full(BASIC_QUEUE_t * queue, bool * full);
int basic_queue_enqueue(BASIC_QUEUE_t * queue, const void * data);
int basic_queue_dequeue(BASIC_QUEUE_t * queue, void * data);
int basic_queue_enqueue_no_block(BASIC_QUEUE_t * queue, const void * data);
int basic_queue_dequeue_no_block(BASIC_QUEUE_t * queue, void * data);

#endif
