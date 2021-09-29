#ifndef LOG_H
#define LOG_H

#include "etsoc/common/log_common.h"

#include <stdint.h>
#include <stdio.h>

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
    \param str Pointer to a string, Maximum length of string is 64 bytes.
    \return bytes written
*/
int64_t log_write_str(log_level_t level, const char *str);

#endif
