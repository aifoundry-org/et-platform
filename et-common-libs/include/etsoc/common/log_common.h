#ifndef _LOG_COMMON_H_
#define _LOG_COMMON_H_

#include <stdint.h>

/*
 * Log Interface.
 */
typedef uint8_t log_interface_t;

/*
 * Available Log Interfaces.
 */
enum log_interface_t {
    LOG_DUMP_TO_TRACE = 0,
    LOG_DUMP_TO_UART = 1,
};

/*
 * Log Level.
 */
typedef uint8_t log_level_t;

/*
 * Available log levels.
 */
typedef enum {
    LOG_LEVEL_CRITICAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level_e;

#endif /* _LOG_COMMON_H_ */
