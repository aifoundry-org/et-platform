/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file log.h
    \brief A C header that defines public interfaces log services
*/
/***********************************************************************/
#ifndef LOG_DEFS_H
#define LOG_DEFS_H

#include <stddef.h>
#include <inttypes.h>

/* mm_rt_svcs */
#include <etsoc/common/log_common.h>

/* mm specific headers */
#include "device_minion_runtime_build_configuration.h"

#if defined(DEVICE_MINION_RUNTIME_BUILD_RELEASE)
#if TEST_FRAMEWORK == 1
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#else
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR
#endif
#elif defined(DEVICE_MINION_RUNTIME_BUILD_DEBUG)
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(DEVICE_MINION_RUNTIME_BUILD_INFO)
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

/*! \def LOG_STRING_MAX_SIZE_MM
    \brief Max string message length which can be logged over serial.
*/
#define LOG_STRING_MAX_SIZE_MM 128

/*! \fn void Log_Init(void)
    \brief Initialize the logging component
    \return none
*/
void Log_Init(void);

/*! \fn void Log_Set_Interface(log_interface_t interface)
    \brief Set current log interface
    \return none
*/
void Log_Set_Interface(log_interface_t interface);

/*! \fn log_interface_t Log_Get_Interface(void)
    \brief Get current log interface
    \return The current log interface
*/
log_interface_t Log_Get_Interface(void);

/*! \fn log_interface_t Log_Get_Level(void)
    \brief Get current log level
    \return The current log level
*/
log_level_t Log_Get_Level(void);

/*! \fn int32_t __Log_Write(const char *const fmt, ...)
    \brief Write a log with va_list style args without any restriction by log level
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return Bytes written
*/
int32_t __Log_Write(log_level_e level, const char *const fmt, ...)
    __attribute__((format(printf, 2, 3)));

/*! \fn int32_t Log_Write_String(const char *str, size_t length)
    \brief Write a string log without any restriction by log level
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return bytes written
*/
int32_t __Log_Write_String(log_level_e level, const char *str, size_t length);

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
#define Log_Write_String(level, str, length)        \
    do                                              \
    {                                               \
        if (level <= CURRENT_LOG_LEVEL)             \
            __Log_Write_String(level, str, length); \
    } while (0)

#define ASSERT_LOG(log_level, msg, expr)                                                \
    do                                                                                  \
    {                                                                                   \
        if (!(expr))                                                                    \
            Log_Write(log_level, "%s || File:%s Line:%d\r\n", msg, __FILE__, __LINE__); \
    } while (0)

/* MM Trace eviction level */
#define LOG_MM_TRACE_EVICT_LEVEL LOG_LEVEL_ERROR

#endif /* LOG1_DEFS_H */
