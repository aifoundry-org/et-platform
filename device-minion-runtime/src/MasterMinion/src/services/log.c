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
#include "services/trace.h"
#include "device-common/hart.h"
#include "drivers/console.h"
#include "atomic.h"
#include "layout.h"
#include <stddef.h>
#include "sync.h"

/*
 * Log control block for current logging information. 
 */
typedef struct log_cb {
    union {
        struct {
            uint8_t  current_log_level;
            uint8_t  current_log_interface;
        };
        uint16_t raw_log_info;
    };
} log_cb_t;

/*! \var log_level_t Log_CB
    \brief Global variable that maintains the current log information
    \warning Not thread safe!
*/
static log_cb_t Log_CB __attribute__((aligned(64))) = 
        {.current_log_level = LOG_LEVEL_WARNING, .current_log_interface = LOG_DUMP_TO_UART};

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
    atomic_store_local_8(&Log_CB.current_log_level, level);
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
    return atomic_load_local_8(&Log_CB.current_log_level);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Set_Interface
*
*   DESCRIPTION
*
*       Set interface to dump log messages.
*
*   INPUTS
*
*       log_interface_t   Interface to dump log messages.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Log_Set_Interface(log_interface_t interface)
{
    atomic_store_local_8(&Log_CB.current_log_interface, interface);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Get_Interface
*
*   DESCRIPTION
*
*       Get current log interface.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       log_interface_t    returns the current log interface.
*
***********************************************************************/
log_interface_t Log_Get_Interface(void)
{
    return atomic_load_local_8(&Log_CB.current_log_interface);
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
    int32_t bytes_written=0;
    log_cb_t log_cb;
    log_cb.raw_log_info = atomic_load_local_16(&Log_CB.raw_log_info);

    /* Dump the log message over current log interface. */
    if (log_cb.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        /* TODO: Use Trace_Format_String() when Trace common library has support/alternative
           of libc_nano to process va_list string formatting. Also, remove log_level check 
           from here, as trace library does that internally .*/
        if(CHECK_STRING_FILTER(Trace_Get_MM_CB(), level))
        {
            va_start(va, fmt);
            vsnprintf(buff, sizeof(buff), fmt, va);

            Trace_String(level, Trace_Get_MM_CB(), buff);

            /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string 
            type message. */
            bytes_written = TRACE_STRING_MAX_SIZE;
        }
    }
    else
    {
        if (level > log_cb.current_log_level) 
        {
            return 0;
        }

        va_start(va, fmt);
        vsnprintf(buff, sizeof(buff), fmt, va);

        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        bytes_written = SERIAL_puts(UART0, buff);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

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
    log_cb_t log_cb;
    log_cb.raw_log_info = atomic_load_local_16(&Log_CB.raw_log_info);

    /* Dump the log message over current log interface. */
    if (log_cb.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        Trace_String(level, Trace_Get_MM_CB(), str);

        /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string 
           type message. */
        bytes_written = TRACE_STRING_MAX_SIZE;
    }
    else
    {
        if (level > log_cb.current_log_level) 
        {
            return 0;
        }

        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        bytes_written = SERIAL_write(UART0, str, (int)length);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return bytes_written;
}
