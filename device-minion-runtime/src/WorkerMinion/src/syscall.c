#include "syscall.h"
#include "kernel.h"
#include "log.h"
#include "message.h"

#include <stdint.h>

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t rv;

    switch (number)
    {
        case SYSCALL_BROADCAST:
        case SYSCALL_IPI_TRIGGER:
        case SYSCALL_INIT_L1:
        case SYSCALL_GET_MTIME:
        case SYSCALL_DRAIN_COALESCING_BUFFER:
        case SYSCALL_EVICT_L1:
        case SYSCALL_UNLOCK_L1:
        case SYSCALL_EVICT_L2:
        case SYSCALL_EVICT_L2_WAIT:
        case SYSCALL_CACHE_CONTROL:
        case SYSCALL_FLUSH_L3:
        case SYSCALL_FLUSH_L3_ALT:
        case SYSCALL_ENABLE_THREAD1:
        case SYSCALL_DRAIN_COALESCING_BUFFER_ALT:
        case SYSCALL_CACHE_CONTROL_ALT:
            rv = syscall(number, arg1, arg2, arg3); // forward the syscall to the machine mode handler
        break;

        case SYSCALL_RETURN_FROM_KERNEL:
            rv = return_from_kernel((int64_t)arg1);
        break;

        case SYSCALL_LOG_WRITE:
            rv = log_write(arg1, (char*)arg2, arg3);
        break;

        case SYSCALL_GET_LOG_LEVEL:
            rv = (int64_t)get_log_level();
        break;

        case SYSCALL_MESSAGE_SEND:
            rv = message_send_worker(arg1, arg2, (message_t*)arg3);
        break;

        case SYSCALL_PRE_KERNEL_SETUP:
        case SYSCALL_POST_KERNEL_CLEANUP:
            rv = -1; // user mode should never ask supervisor worker minon firmware to do this
        break;

        default:
            rv = -1; // unhandled syscall
        break;
    }

    return rv;
}
