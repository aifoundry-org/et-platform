#include "atomic.h"

#include <stdint.h>

static inline __attribute__((always_inline)) void atomic_barrier_init(int64_t* const counter_ptr, int64_t value)
{
    atomic_write(counter_ptr, value);
}

// Decrements counter_ptr
// Spins until counter_ptr is <= 0
static inline __attribute__((always_inline)) void atomic_barrier(int64_t* const counter_ptr)
{
    // if result is > 0, block until counter is <= 0
    if (atomic_add(counter_ptr, -1) > 1)
    {
        while (atomic_read(counter_ptr) > 0) {}
    }

    return;
}
