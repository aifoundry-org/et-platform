#include "syscall_internal.h"
#include "kernel.h"

#include <stdint.h>

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t ret = SYSCALL_SUCCESS;

    switch (number)
    {
        case SYSCALL_CACHE_OPS_EVICT_SW:
            ret = syscall(SYSCALL_CACHE_OPS_EVICT_SW_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_CACHE_OPS_FLUSH_SW:
            ret = syscall(SYSCALL_CACHE_OPS_FLUSH_SW_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_CACHE_OPS_LOCK_SW:
            ret = syscall(SYSCALL_CACHE_OPS_LOCK_SW_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_CACHE_OPS_UNLOCK_SW:
            ret = syscall(SYSCALL_CACHE_OPS_UNLOCK_SW_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_CACHE_OPS_INVALIDATE:
            ret = syscall(SYSCALL_CACHE_OPS_INVALIDATE_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_CACHE_OPS_EVICT_L1:
            ret = syscall(SYSCALL_CACHE_OPS_EVICT_L1_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_SHIRE_CACHE_BANK_OP:
            ret = syscall(SYSCALL_SHIRE_CACHE_BANK_OP_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_RETURN_FROM_KERNEL:
            /* Dump U-mode context in case of kernel self abort */
            if(arg2 == KERNEL_RETURN_SELF_ABORT)
            {
                kernel_self_abort_save_context();
            }
            ret = return_from_kernel((int64_t)arg1, arg2);
            break;
        case SYSCALL_CACHE_CONTROL:
            ret = syscall(SYSCALL_CACHE_CONTROL_INT, arg1, arg2, arg3);
            break;
        case SYSCALL_FLUSH_L3:
            ret = syscall(SYSCALL_FLUSH_L3_INT, arg1, arg2, arg3);
            break;
        default:
            ret = SYSCALL_INVALID_ID;
            break;
    }

    return ret;
}
