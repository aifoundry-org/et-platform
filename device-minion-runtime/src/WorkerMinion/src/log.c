#include "atomic.h"
#include "layout.h"
#include "log.h"
#include "printf.h"
#include "serial.h"
#include "sync.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

// TODO All worker harts currently crudely share the same log level
static log_level_t current_log_level __attribute__((aligned(64))) = LOG_LEVEL_WARNING;

void log_set_level(log_level_t level)
{
    atomic_store_global_8(&current_log_level, level);
}

log_level_t get_log_level(void)
{
    return atomic_load_global_8(&current_log_level);
}

// print a log message to the UART for display
int64_t log_write(log_level_t level, const char *const fmt, ...)
{
    int ret;
    va_list va;
    char buff[128];

    if (level > atomic_load_global_8(&current_log_level)) {
        return 0;
    }

    va_start(va, fmt);
    vsnprintf(buff, sizeof(buff), fmt, va);

    acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    ret = SERIAL_puts(UART0, buff);
    release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);

    return ret;
}

int64_t log_write_str(log_level_t level, const char *str, size_t length)
{
    int ret;

    if (level > atomic_load_global_8(&current_log_level)) {
        return 0;
    }

    acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    ret = SERIAL_write(UART0, str, (int)length);
    release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);

    return ret;
}
