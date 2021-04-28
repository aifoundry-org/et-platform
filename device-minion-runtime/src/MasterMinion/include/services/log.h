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
#include "device-common/log_common.h"

/*! \fn void Log_Init(void)
    \brief Initialize the logging component
    \param level Log level to set
    \return none
*/
void Log_Init(log_level_t level);

/*! \fn void Log_Set_Level(log_level_t level)
    \brief Set the current global log level
    \param level Log level to set
    \return none
*/
void Log_Set_Level(log_level_t level);

/*! \fn log_level_t Log_Get_Level(void)
    \brief Get current global log level
    \return The current global log level
*/
log_level_t Log_Get_Level(void);

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

/*! \fn int32_t Log_Write(log_level_t level, const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param level Log level for the current log
    \param fmt format specifier
    \param ... variable list
    \return Bytes written
*/
int32_t Log_Write(log_level_t level, const char *const fmt, ...) __attribute__((format(printf, 2, 3)));

/*! \fn int32_t Log_Write_String(log_level_t level, const char *str, size_t length)
    \brief Write a string log
    \param level Log level for the current log
    \param str Pointer to a string
    \param length Length of string
    \return bytes written
*/
int32_t Log_Write_String(log_level_t level, const char *str, size_t length);

/* TODO: Redefining the shared header signiture with code convention conformant
definitions, log.h is used by many components, remove this redifinition
once the shared log.h is made conformant to code convention */
#define log_set_level   Log_Set_Level
#define get_log_level   Log_Get_Level
#define log_write       Log_Write
#define log_write_str   Log_Write_String

#define ASSERT_LOG(log_level,msg,expr) if (!(expr)) Log_Write(log_level,"%s || File:%s Line:%d\r\n", msg, __FILE__, __LINE__)

#endif /* LOG1_DEFS_H */
