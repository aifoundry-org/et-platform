#ifndef SYSCALL_INTERNAL_H
#define SYSCALL_INTERNAL_H

#ifndef __ASSEMBLER__
#include "device-common/syscall.h"
#endif

#define SYSCALL_BROADCAST_INT                 1
#define SYSCALL_IPI_TRIGGER_INT               2
#define SYSCALL_ENABLE_THREAD1_INT            3
#define SYSCALL_PRE_KERNEL_SETUP_INT          4
#define SYSCALL_POST_KERNEL_CLEANUP_INT       5
#define SYSCALL_GET_MTIME_INT                 6
#define SYSCALL_CACHE_CONTROL_INT             7
#define SYSCALL_FLUSH_L3_INT                  8
#define SYSCALL_EVICT_L3_INT                  9
#define SYSCALL_SHIRE_CACHE_BANK_OP_INT      10
#define SYSCALL_CONFIGURE_PMCS_INT           11
#define SYSCALL_RESET_PMCS_INT               12
#define SYSCALL_SAMPLE_PMCS_INT              13
#define SYSCALL_CONFIGURE_COMPUTE_MINION     14
#define SYSCALL_CACHE_OPS_EVICT_SW_INT       15
#define SYSCALL_CACHE_OPS_FLUSH_SW_INT       16
#define SYSCALL_CACHE_OPS_LOCK_SW_INT        17
#define SYSCALL_CACHE_OPS_UNLOCK_SW_INT      18
#define SYSCALL_CACHE_OPS_INVALIDATE_INT     19
#define SYSCALL_CACHE_OPS_EVICT_L1_INT       20
#define SYSCALL_UPDATE_MINION_PLL_FREQUENCY  21

#endif // SYSCALL_INTERNAL_H
