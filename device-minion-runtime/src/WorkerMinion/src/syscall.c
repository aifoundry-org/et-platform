#include "syscall_internal.h"
#include "kernel.h"
#include "log.h"

#include <stdint.h>

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t ret = 0;

    switch (number) {
    case SYSCALL_CACHE_CONTROL:
        ret = syscall(SYSCALL_CACHE_CONTROL_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_FLUSH_L3:
        ret = syscall(SYSCALL_FLUSH_L3_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_SHIRE_CACHE_BANK_OP:
        ret = syscall(SYSCALL_SHIRE_CACHE_BANK_OP_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_EVICT_L1:
        ret = syscall(SYSCALL_EVICT_L1_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_RETURN_FROM_KERNEL:
        ret = return_from_kernel((int64_t)arg1);
        break;
    case SYSCALL_LOG_WRITE:
        ret = log_write_str(LOG_LEVEL_CRITICAL, (const char *)arg1, (size_t)arg2);
        break;
    case SYSCALL_GET_LOG_LEVEL:
        ret = (int64_t)get_log_level();
        break;
    case SYSCALL_GET_MTIME:
        ret = syscall(SYSCALL_GET_MTIME_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_CONFIGURE_PMCS:
        ret = syscall(SYSCALL_CONFIGURE_PMCS_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_SAMPLE_PMCS:
        ret = syscall(SYSCALL_SAMPLE_PMCS_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_RESET_PMCS:
        ret = syscall(SYSCALL_RESET_PMCS_INT, arg1, arg2, arg3);
        break;
    default:
        ret = -1; // unhandled syscall
        break;
    }

    return ret;
}
