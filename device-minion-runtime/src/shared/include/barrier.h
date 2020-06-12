#ifndef BARRIER_H
#define BARRIER_H

#include <stdint.h>
#include <stdbool.h>
#include "atomic.h"
#include "fcc.h"
#include "macros.h"

#define CACHE_LINE_SIZE 64

typedef struct {
    uint64_t in;
    uint64_t out;
} __attribute__((aligned(CACHE_LINE_SIZE))) local_fcc_barrier_t;

static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
{
    atomic_store_local_64(&barrier->in, 0);
    atomic_store_local_64(&barrier->out, 0);
}

static inline bool local_fcc_barrier(local_fcc_barrier_t *barrier, uint64_t thread_count, uint64_t minion_mask)
{
    if (atomic_add_local_64(&barrier->in, 1) == thread_count - 1) {
        while (atomic_load_local_64(&barrier->out) != (thread_count - 1)) {
            SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, minion_mask);
        }
        atomic_add_local_64(&barrier->out, 1);
        return true;
    } else {
        do {
            WAIT_FCC(FCC_0);
        } while (atomic_load_local_64(&barrier->in) != thread_count);

        atomic_add_local_64(&barrier->out, 1);
	while (atomic_load_local_64(&barrier->out) != thread_count) {
	    FENCE
        }
        return false;
    }
}

#endif
