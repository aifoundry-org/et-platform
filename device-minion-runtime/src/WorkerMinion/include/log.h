#ifndef LOG_H
#define LOG_H

#include <stddef.h>
#include <inttypes.h>

/* cm specific headers */
#include <etsoc/common/log_common.h>
#include "device_minion_runtime_build_configuration.h"
#include "trace.h"

/* CM Trace eviction level */
#define LOG_CM_TRACE_EVICT_LEVEL LOG_LEVEL_ERROR

#if defined(DEVICE_MINION_RUNTIME_BUILD_RELEASE)
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR
#elif defined(DEVICE_MINION_RUNTIME_BUILD_DEBUG)
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(DEVICE_MINION_RUNTIME_BUILD_INFO)
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

/*! \fn int32_t Log_Write(log_level_t level, const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return None
*/
#define Log_Write(level, fmt, ...)                                             \
    do                                                                         \
    {                                                                          \
        if (level <= CURRENT_LOG_LEVEL)                                        \
        {                                                                      \
            Trace_Format_String(level, Trace_Get_CM_CB(), fmt, ##__VA_ARGS__); \
                                                                               \
            if (level <= LOG_CM_TRACE_EVICT_LEVEL)                             \
            {                                                                  \
                Trace_Evict_CM_Buffer();                                       \
            }                                                                  \
        }                                                                      \
    } while (0)

/*! \fn int32_t Log_Write_String(log_level_t level, const char *str, size_t length)
    \brief Write a string log
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return None
*/
#define Log_Write_String(level, str, length)             \
    do                                                   \
    {                                                    \
        if (level <= CURRENT_LOG_LEVEL)                  \
        {                                                \
            Trace_String(level, Trace_Get_CM_CB(), str); \
                                                         \
            if (level <= LOG_CM_TRACE_EVICT_LEVEL)       \
            {                                            \
                Trace_Evict_CM_Buffer();                 \
            }                                            \
        }                                                \
    } while (0)

#endif
