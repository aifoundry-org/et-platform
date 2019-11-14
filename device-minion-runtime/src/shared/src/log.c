#include "log.h"
#include "hart.h"
#include "message.h"
#include "printf.h"
#include "syscall.h"

#include <stdarg.h>
#include <stddef.h>

// sends a log message from a worker minion in user privilege to the master minion for display
// Not intended for firmware use - only call from kernels in user mode
int64_t log_write(log_level_t level, const char* const fmt, ...)
{
    // TODO two syscalls is more overhead, but can't pass va_args through syscall and should do
    // string manipulation in lowest privilege possible. Could check current_log_level in syscall
    // after building string and drop there.
    const log_level_t current_log_level = (log_level_t)syscall(SYSCALL_GET_LOG_LEVEL, 0, 0, 0);

    if (level > current_log_level)
    {
        return 0;
    }

    message_t message;
    message.id = MESSAGE_ID_LOG_WRITE;

    va_list va;
    va_start(va, fmt);
    char* string_ptr = (char*)message.data;

    if (vsnprintf(string_ptr, sizeof(message.data), fmt, va) < 0)
    {
        return -1;
    }

    return syscall(SYSCALL_MESSAGE_SEND, get_shire_id(), get_hart_id(), (uint64_t)&message);
}
