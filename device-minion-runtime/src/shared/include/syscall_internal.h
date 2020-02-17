#ifndef SYSCALL_INTERNAL_H
#define SYSCALL_INTERNAL_H

#include <stdint.h>
#include "syscall.h"

enum {
    // From S-mode to M-mode [0-299]
    SYSCALL_BROADCAST_INT               = 0,
    SYSCALL_IPI_TRIGGER_INT             = 1,
    SYSCALL_ENABLE_THREAD1_INT          = 2,
    SYSCALL_PRE_KERNEL_SETUP_INT        = 3,
    SYSCALL_POST_KERNEL_CLEANUP_INT     = 4,
    SYSCALL_GET_MTIME_INT               = 7,
    SYSCALL_DRAIN_COALESCING_BUFFER_INT = 8,
    SYSCALL_CACHE_CONTROL_INT           = 9,
    SYSCALL_FLUSH_L3_INT                = 10,
    SYSCALL_SHIRE_CACHE_BANK_OP_INT     = 11,
    SYSCALL_EVICT_L1_INT                = 12
};

#endif // SYSCALL_INTERNAL_H
