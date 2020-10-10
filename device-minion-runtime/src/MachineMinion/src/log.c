#include "log.h"
#include "printf.h"

#include <stddef.h>

static log_level_t current_log_level = LOG_LEVEL_WARNING;

void log_set_level(log_level_t level)
{
    current_log_level = level;
}

log_level_t get_log_level(void)
{
    return current_log_level;
}

// print a log message to the UART for display
int64_t log_write(log_level_t level, const char *const fmt, ...)
{
    if (level > current_log_level) {
        return 0;
    }

    va_list va;
    va_start(va, fmt);
    //return vprintf(fmt, va); // What do we do in Machine mode?
    return 0;
}

int64_t log_write_str(log_level_t level, const char *str, size_t length)
{
    size_t i;

    if (level > current_log_level) {
        return 0;
    }

    for (i = 0; i < length && str[i]; i++) {
        // _putchar(str[i]); // What do we do in Machine mode?
    }

    return (int64_t)i;
}
