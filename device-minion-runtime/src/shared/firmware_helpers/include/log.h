#ifndef LOG_H
#define LOG_H

#include "device-common/log_levels.h"

#include <stdint.h>
#include <stdio.h>

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

/*! \fn void log_set_interface(log_interface_t interface)
    \brief Set current log interface
    \return none
*/
void log_set_interface(log_interface_t interface);

/*! \fn log_interface_t log_get_interface(void)
    \brief Get current log interface
    \return The current log interface
*/
log_interface_t log_get_interface(void);

/*! \fn void log_set_level(log_level_t level)
    \brief Set the current global log level
    \param level Log level to set
    \return none
*/
void log_set_level(log_level_t level);

/*! \fn log_level_t log_get_level(void)
    \brief Get current global log level
    \return The current global log level
*/
log_level_t log_get_level(void);

/*! \fn int32_t log_write(log_level_t level, const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return Bytes written
*/
int64_t log_write(log_level_t level, const char *const fmt, ...);

/*! \fn int32_t log_write_str(log_level_t level, const char *str, size_t length)
    \brief Write a string log
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return bytes written
*/
int64_t log_write_str(log_level_t level, const char *str, size_t length);

#endif
