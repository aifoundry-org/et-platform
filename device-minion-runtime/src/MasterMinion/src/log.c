#include "log.h"
#include "printf.h"

#include <stddef.h>

static log_level_t current_log_level = LOG_LEVEL_INFO;

void log_set_level(log_level_t level)
{
    current_log_level = level;
}

log_level_t get_log_level(void)
{
    return current_log_level;
}

// print a log message to the UART for display
int64_t log_write(log_level_t level, const char* const fmt, ...)
{
    if (level <= current_log_level)
    {
        va_list va;
        va_start(va, fmt);
        vprintf(fmt, va);
    }

    return 0;
}
