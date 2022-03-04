#ifndef LOG_H
#define LOG_H

#include <stddef.h>
#include <inttypes.h>

/* cm specific headers */
#include <etsoc/common/log_common.h>
#include "device_minion_runtime_build_configuration.h"

/* CM Trace eviction level */
#define LOG_CM_TRACE_EVICT_LEVEL LOG_LEVEL_ERROR

#if defined(DEVICE_MINION_RUNTIME_BUILD_RELEASE)
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR
#elif defined(DEVICE_MINION_RUNTIME_BUILD_DEBUG)
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(DEVICE_MINION_RUNTIME_BUILD_INFO)
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

/*! \fn int32_t __Log_Write(const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return Bytes written
*/
int32_t __Log_Write(log_level_e level, const char *const fmt, ...)
    __attribute__((format(printf, 2, 3)));

/*! \fn int32_t __Log_Write_Str(const char *str, size_t length)
    \brief Write a string log without any restriction by log level
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return bytes written
*/
int32_t __Log_Write_Str(log_level_e level, const char *str, size_t length);

/*! \fn int32_t Log_Write(log_level_t level, const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return Bytes written
*/
#define Log_Write(level, fmt, ...)                  \
    do                                              \
    {                                               \
        if (level <= CURRENT_LOG_LEVEL)             \
            __Log_Write(level, fmt, ##__VA_ARGS__); \
    } while (0)

/*! \fn int32_t Log_Write_String(const char *str, size_t length)
    \brief Write a string log
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return bytes written
*/
#define log_write_str(level, str, length)        \
    do                                           \
    {                                            \
        if (level <= CURRENT_LOG_LEVEL)          \
            __Log_Write_Str(level, str, length); \
    } while (0)

#endif
