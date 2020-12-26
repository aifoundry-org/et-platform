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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for logging services.
*
***********************************************************************/
#ifndef LOG1_DEFS_H
#define LOG1_DEFS_H

#include "log.h"

/*! \fn void Log_Set_Level(log_level_t level)
    \brief Set the current global log level
    \param [in] Log level to set
*/
void Log_Set_Level(log_level_t level);

/*! \fn log_level_t Log_Get_Level(void)
    \brief Get log level
    \param [out] Get the current global log level
*/
log_level_t Log_Get_Level(void);

/*! \fn int64_t Log_Write(log_level_t level, const char *const fmt, ...)
    \brief Write a log with va_list style args
    \param [in] Log level for the current log
    \param [in] format specifier
    \param [in] ... variable list
*/
int64_t Log_Write(log_level_t level, const char *const fmt, ...);

/*! \fn int64_t Log_Write_String(log_level_t level, const char *str, size_t length)
    \brief Write a string log
    \param [in] Log level for the current log
    \param [in] Pointer to a string
    \param [in] Length of string
*/
int64_t Log_Write_String(log_level_t level, const char *str, size_t length);

/* TODO: Redefining the shared header signiture with code convention conformant
definitions, log.h is used by many components, remove this redifinition
once the shared log.h is made conformant to code convention */
#define log_set_level   Log_Set_Level
#define get_log_level   Log_Get_Level   
#define log_write       Log_Write   
#define log_write_str   Log_Write_String

#define ASSERT_LOG(log_level,msg,expr) if (!(expr)) Log_Write(log_level,"%s || File:%s Line:%d \r\n", msg, __FILE__, __LINE__)

#endif /* LOG1_DEFS_H */