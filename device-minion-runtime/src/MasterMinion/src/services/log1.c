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
*       This file consists the implementation for logging services
*
*   FUNCTIONS
*
*       Log_Set_Level
*       Log_Get_Level
*       Log_Write
*       Log_Write_String
*
***********************************************************************/
#include "services/log1.h"
#include "drivers/console.h"
#include <stddef.h>

/* TODO: Debug code to serialize concurrent printing */
#include "sync.h"

/*! \var log_level_t Current_Log_Level
    \brief Global variable that maintains the current log level
    \warning Not thread safe!
*/
static log_level_t Current_Log_Level = LOG_LEVEL_WARNING;

/* TODO: Debug code to serialize concurrent printing */
static spinlock_t Console_Lock = 0;

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
    Current_Log_Level = level;
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
    return Current_Log_Level;
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
/* TODO: @ET why should this be a int64 ? since this header is used my multiple
components I am leaving this the way it is, please ocnsider making the return
status a int32_t */
int64_t Log_Write(log_level_t level, const char *const fmt, ...)
{
    int64_t bytes_written;
    
    /* TODO: Debug code to serialize console printing */
    acquire_local_spinlock(&Console_Lock);

    if (level > Current_Log_Level) {
        return 0;
    }
    
    va_list va;
    va_start(va, fmt);

    bytes_written = vprintf(fmt, va);
    
    /* TODO: Debug code to serialize console printing */
    release_local_spinlock(&Console_Lock);

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
/* TODO: @ET why should this be a int64 ? since this header is used my multiple
components I am leaving this the way it is, please ocnsider making the return
status a int32_t */
int64_t Log_Write_String(log_level_t level, const char *str, size_t length)
{
    size_t i;

    /* TODO: Debug code to serialize console printing */
    acquire_local_spinlock(&Console_Lock);

    if (level > Current_Log_Level) {
        return 0;
    }

    for (i = 0; i < length && str[i]; i++) {
        Console_Putchar(str[i]);
    }

    /* TODO: Debug code to serialize console printing */
    release_local_spinlock(&Console_Lock);

    return (int64_t)i;
}
