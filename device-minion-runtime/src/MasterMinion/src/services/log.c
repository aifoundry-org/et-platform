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
/*! \file log.c
    \brief A C module that implements the logging services

    Public interfaces:
        Log_Init
        Log_Set_Level
        Log_Get_Level
        Log_Write
        Log_Write_String
*/
/***********************************************************************/
#include "services/log.h"
#include "drivers/console.h"
#include "atomic.h"
#include "layout.h"
#include <stddef.h>
#include "sync.h"

/*! \var log_level_t Current_Log_Level
    \brief Global variable that maintains the current log level
    \warning Not thread safe!
*/
static log_level_t Current_Log_Level __attribute__((aligned(64))) = LOG_LEVEL_WARNING;

/************************************************************************
*
*   FUNCTION
*
*       Log_Init
*
*   DESCRIPTION
*
*       Initialize the logging
*
*   INPUTS
*
*       log_level_t   log level to set
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Log_Init(log_level_t level)
{
    /* Init console lock to released state */
    init_global_spinlock((spinlock_t*)FW_GLOBAL_UART_LOCK_ADDR, 0);

    /* Initialize the log level */
    Log_Set_Level(level);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Set_Level
*
*   DESCRIPTION
*
*       Set logging level
*
*   INPUTS
*
*       log_level_t   log level to set
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Log_Set_Level(log_level_t level)
{
    atomic_store_local_8(&Current_Log_Level, level);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Get_Level
*
*   DESCRIPTION
*
*       Get logging level
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       log_level_t    returns the log level
*
***********************************************************************/
log_level_t Log_Get_Level(void)
{
    return atomic_load_local_8(&Current_Log_Level);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Write
*
*   DESCRIPTION
*
*       Write a va_list style payload to serial port
*
*   INPUTS
*
*       log_level_t    log level
*       const chat*    format specifier
*       ...            variable argument list
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t Log_Write(log_level_t level, const char *const fmt, ...)
{
    char buff[128];
    va_list va;
    int32_t bytes_written;

    if (level > atomic_load_local_8(&Current_Log_Level)) {
        return 0;
    }

    va_start(va, fmt);
    vsnprintf(buff, sizeof(buff), fmt, va);

    acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    bytes_written = SERIAL_puts(UART0, buff);
    release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);

    return bytes_written;
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Write_String
*
*   DESCRIPTION
*
*       Write a string of
*
*   INPUTS
*
*       log_level_t    log level
*       char*          pointer to string
*       size_t         length of string
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t Log_Write_String(log_level_t level, const char *str, size_t length)
{
    int32_t bytes_written = 0;

    if (level > atomic_load_local_8(&Current_Log_Level)) {
        return 0;
    }

    acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    bytes_written = SERIAL_write(UART0, str, (int)length);
    release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);

    return bytes_written;
}
