#ifndef KERNEL_SYNC_H
#define KERNEL_SYNC_H

#include "layout.h"
#include "fcc.h"

#include <stdint.h>

// First minion in the master shire dedicated to kernel launch synchronization
// kernel ID 0 maps to this minion thread 0, 1 to this minion thread 1, etc.
#define FIRST_KERNEL_LAUNCH_SYNC_MINON 1

// Sends a FCC to the appropriate kernel sync thread (HART) for the kernel_id
static inline void notify_kernel_sync_thread(uint64_t kernel_id, fcc_t fcc)
{
    const uint64_t bitmask = 1U << (FIRST_KERNEL_LAUNCH_SYNC_MINON + (kernel_id / 2));
    const uint64_t thread = kernel_id % 2;

    // Last thread to join barrier sends ready FCC1 to master shire sync HART
    SEND_FCC(MASTER_SHIRE, thread, fcc, bitmask);
}

#endif
