#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "basic_queue.h"
/*
static int relative_to_absolute_time(struct timespec * time, uint32_t delta_ms) {
    time_t seconds = delta_ms / 1000;
    long nanoseconds = (delta_ms % 1000) * 1000000;

    if (0 != clock_gettime(CLOCK_REALTIME, time)) {
        return -1;
    }

    time->tv_nsec += nanoseconds;
    if (time->tv_nsec > 999999999) {
        time->tv_nsec -= 1000000000;
        time->tv_sec++;
    }

    time->tv_sec += seconds;
    return 0;
}
*/
int basic_queue_create(BASIC_QUEUE_t * queue, size_t element_size, size_t elements_count) {
    if (0 != pthread_mutex_init(&queue->queue_mutex, NULL)) {
        return -1;
    }

    if (0 != pthread_cond_init(&queue->queue_is_not_empty_cond, NULL)) {
        return -1;
    }

    if (0 != pthread_cond_init(&queue->queue_is_not_full_cond, NULL)) {
        return -1;
    }

    queue->queue_data = (uint8_t*)malloc(element_size * elements_count);
    if (NULL == queue->queue_data) {
        return -1;
    }
    queue->element_size = element_size;
    queue->elements_count = elements_count;
    queue->head_index = 0;
    queue->tail_index = 0;

    return 0;
}

int basic_queue_delete(BASIC_QUEUE_t * queue) {
    pthread_cond_destroy(&queue->queue_is_not_full_cond);
    pthread_cond_destroy(&queue->queue_is_not_empty_cond);
    pthread_mutex_destroy(&queue->queue_mutex);
    free(queue->queue_data);

    return 0;
}

static bool _queue_is_empty(BASIC_QUEUE_t * queue) {
    if (queue->head_index == queue->tail_index) {
        return true;
    } else {
        return false;
    }
}

static bool _queue_is_full(BASIC_QUEUE_t * queue) {
    uint32_t next_head_index = (uint32_t)((queue->head_index + 1) % queue->elements_count);
    if (next_head_index == queue->tail_index) {
        return true;
    } else {
        return false;
    }
}

static bool _queue_push(BASIC_QUEUE_t * queue, const void * data) {
    uint32_t next_head_index = (uint32_t)((queue->head_index + 1) % queue->elements_count);
    if (next_head_index == queue->tail_index) {
        return false;
    }
    memcpy(queue->queue_data + queue->head_index * queue->element_size, data, queue->element_size);
    queue->head_index = next_head_index;
    return true;
}

static bool _queue_pop(BASIC_QUEUE_t * queue, void * data) {
    if (queue->head_index == queue->tail_index) {
        return false;
    }

    memcpy(data, queue->queue_data + queue->tail_index * queue->element_size, queue->element_size);
    queue->tail_index = (queue->tail_index + 1) % queue->elements_count;
    return true;
}

int basic_queue_is_empty(BASIC_QUEUE_t * queue, bool * empty) {
    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("queue_is_empty: pthread_mutex_lock() failed!\n");
        return -1;
    }

    *empty = _queue_is_empty(queue);

    if (0 != pthread_mutex_unlock(&queue->queue_mutex)) {
        printf("queue_is_empty: pthread_mutex_unlock() failed!\n");
        return -1;
    }

    return 0;
}

int basic_queue_is_full(BASIC_QUEUE_t * queue, bool * full) {
    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("queue_is_full: pthread_mutex_lock() failed!\n");
        return -1;
    }

    *full = _queue_is_full(queue);

    if (0 != pthread_mutex_unlock(&queue->queue_mutex)) {
        printf("queue_is_full: pthread_mutex_unlock() failed!\n");
        return -1;
    }

    return 0;
}

int basic_queue_enqueue(BASIC_QUEUE_t * queue, const void * data) {
    int rv = 0;
#if 0
    struct timespec abstime;

    if (BASIC_QUEUE_INFINITE_DELAY != delay_ms) {
        if (0 != relative_to_absolute_time(&abstime, delay_ms)) {
            return -1;
        }
    }
#endif

    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("queue_enqueue: pthread_mutex_lock() failed!\n");
        rv = -1;
    } else {
        while (_queue_is_full(queue)) {
            if (0 != pthread_cond_wait(&queue->queue_is_not_full_cond, &queue->queue_mutex)) {
                printf("queue_enqueue: pthread_cond_wait(not_full) failed!\n");
                rv = -1;
                goto DONE;
            }
        }

        if (!_queue_push(queue, data)) {
            printf("queue_enqueue: _queue_push() failed!\n");
            rv = -1;
            goto DONE;
        } 

        if (0 != pthread_cond_signal(&queue->queue_is_not_empty_cond)) {
            printf("queue_enqueue: pthread_cond_signal(not_full) failed!\n");
            rv = -1;
            goto DONE;
        }

DONE:
        if (pthread_mutex_unlock(&queue->queue_mutex)) {
            printf("queue_enqueue: pthread_mutex_unlock() failed!\n");
            rv = -1;
        }
    }

    return rv;
}

int basic_queue_dequeue(BASIC_QUEUE_t * queue, void * data) {
    int rv = 0;
#if 0
    struct timespec abstime;

    if (BASIC_QUEUE_INFINITE_DELAY != delay_ms) {
        if (0 != relative_to_absolute_time(&abstime, delay_ms)) {
            return -1;
        }
    }
#endif

    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("queue_dequeue: pthread_mutex_lock() failed!\n");
        rv = -1;
    } else {
        while (_queue_is_empty(queue)) {
            if (0 != pthread_cond_wait(&queue->queue_is_not_empty_cond, &queue->queue_mutex)) {
                printf("queue_dequeue: pthread_cond_wait(not_empty) failed!\n");
                rv = -1;
                goto DONE;
            }
        }

        if (!_queue_pop(queue, data)) {
            printf("queue_dequeue: _queue_pop() failed!\n");
            rv = -1;
            goto DONE;
        } 

        if (0 != pthread_cond_signal(&queue->queue_is_not_full_cond)) {
            printf("queue_dequeue: pthread_cond_signal(not_empty) failed!\n");
            rv = -1;
            goto DONE;
        }

DONE:
        if (pthread_mutex_unlock(&queue->queue_mutex)) {
            printf("queue_dequeue: pthread_mutex_unlock() failed!\n");
            rv = -1;
        }
    }

    return rv;
}

int basic_queue_enqueue_no_block(BASIC_QUEUE_t * queue, const void * data) {
    int rv = 0;

    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("basic_queue_enqueue_no_block: pthread_mutex_lock() failed!\n");
        rv = -1;
    } else {
        if (_queue_is_full(queue)) {
            rv = 1;
            goto DONE;
        }

        if (!_queue_push(queue, data)) {
            printf("basic_queue_enqueue_no_block: _queue_push() failed!\n");
            rv = -1;
            goto DONE;
        } 

        if (0 != pthread_cond_signal(&queue->queue_is_not_empty_cond)) {
            printf("basic_queue_enqueue_no_block: pthread_cond_signal(not_full) failed!\n");
            rv = -1;
            goto DONE;
        }

DONE:
        if (pthread_mutex_unlock(&queue->queue_mutex)) {
            printf("basic_queue_enqueue_no_block: pthread_mutex_unlock() failed!\n");
            rv = -1;
        }
    }

    return rv;
}

int basic_queue_dequeue_no_block(BASIC_QUEUE_t * queue, void * data) {
    int rv = 0;
#if 0
    struct timespec abstime;

    if (BASIC_QUEUE_INFINITE_DELAY != delay_ms) {
        if (0 != relative_to_absolute_time(&abstime, delay_ms)) {
            return -1;
        }
    }
#endif

    if (0 != pthread_mutex_lock(&queue->queue_mutex)) {
        printf("basic_queue_dequeue_no_block: pthread_mutex_lock() failed!\n");
        rv = -1;
    } else {
        if (_queue_is_empty(queue)) {
            rv = 1;
            goto DONE;
        }

        if (!_queue_pop(queue, data)) {
            printf("basic_queue_dequeue_no_block: _queue_pop() failed!\n");
            rv = -1;
            goto DONE;
        } 

        if (0 != pthread_cond_signal(&queue->queue_is_not_full_cond)) {
            printf("basic_queue_dequeue_no_block: pthread_cond_signal(not_empty) failed!\n");
            rv = -1;
            goto DONE;
        }

DONE:
        if (pthread_mutex_unlock(&queue->queue_mutex)) {
            printf("basic_queue_dequeue_no_block: pthread_mutex_unlock() failed!\n");
            rv = -1;
        }
    }

    return rv;
}
