#include "syscall_internal.h"
#include "log.h"
#include "message.h"

#include <stdint.h>

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t ret;

    switch (number) {
    case SYSCALL_LOG_WRITE:
        ret = log_write((log_level_t)arg1, (char *)arg2, arg3);
        break;
    case SYSCALL_GET_LOG_LEVEL:
        ret = (int64_t)get_log_level();
        break;
    case SYSCALL_MESSAGE_SEND:
        ret = message_send_worker(arg1, arg2, (message_t *)arg3);
        break;
    case SYSCALL_GET_MTIME:
        ret = syscall(SYSCALL_GET_MTIME_INT, arg1, arg2, arg3);
        break;
    case SYSCALL_CONFIGURE_PMCS:
        ret = syscall(SYSCALL_CONFIGURE_PMCS, arg1, arg2, arg3);
        break;
    case SYSCALL_SAMPLE_PMCS:
        ret = syscall(SYSCALL_SAMPLE_PMCS, arg1, arg2, arg3);
        break;
    case SYSCALL_RESET_PMCS:
        ret = syscall(SYSCALL_RESET_PMCS, arg1, arg2, arg3);
        break;
    default:
        ret = -1; // unhandled syscall
        break;
    }

    return ret;
}
