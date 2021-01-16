#include "log.h"
#include "hart.h"
#include "message.h"
#include "printf.h"
#include "atomic.h"

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

// sends a log message from a worker minion to the master minion for display
int64_t log_write(log_level_t level, const char *const fmt, ...)
{
    if (level > atomic_load_global_8(&current_log_level)) {
        return 0;
    }

    cm_iface_message_t message;
    message.header.id = CM_TO_MM_MESSAGE_ID_LOG_WRITE;

    va_list va;
    va_start(va, fmt);
    char *string_ptr = (char *)message.data;

    if (vsnprintf(string_ptr, sizeof(message.data), fmt, va) < 0) {
        return -1;
    }

    return message_send_worker(get_shire_id(), get_hart_id(), &message);
}

int64_t log_write_str(log_level_t level, const char *str, size_t length)
{
    if (level > atomic_load_global_8(&current_log_level)) {
        return 0;
    }

    while (length > 0 && *str) {
        cm_iface_message_t message;
        message.header.id = CM_TO_MM_MESSAGE_ID_LOG_WRITE;

        char *data = (char *)message.data;
        size_t len;
        for (len = 0; len < length && len < sizeof(message.data) - 1 && str[len]; len++)
            data[len] = str[len];
        data[len] = '\0';

        length -= len;
        str += len;

        int64_t ret = message_send_worker(get_shire_id(), get_hart_id(), &message);
        if (ret < 0)
            return ret;
    }

    return 0;
}
