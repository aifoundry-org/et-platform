#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

#include <stdint.h>

typedef uint8_t log_level_t;

typedef enum {
    LOG_LEVEL_CRITICAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level_e;

#endif
