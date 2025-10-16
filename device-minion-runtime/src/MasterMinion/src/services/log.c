/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file log.c
    \brief A C module that implements the logging services

    Public interfaces:
        Log_Init
        Log_Set_Interface
        Log_Get_Interface
        Log_Get_Level
        __Log_Write
        __Log_Write_String
*/
/***********************************************************************/
#include <stddef.h>

/* mm_rt_svcs */
#include <common/printf.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>

/* mm specific headers */
#include "services/log.h"
#include "services/trace.h"
#include "drivers/console.h"

/*! \var log_interface_t Log_Interface
    \brief Global variable that maintains the current log interface
    \warning Not thread safe!
*/
static log_interface_t Log_Interface __attribute__((aligned(64))) = LOG_DUMP_TO_UART;

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
*       none
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Log_Init()
{
    /* Init console lock to released state */
    init_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR, 0);
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
    atomic_store_local_8(&Log_Interface, interface);
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
    return atomic_load_local_8(&Log_Interface);
}

/************************************************************************
*
*   FUNCTION
*
*       Log_Get_Level
*
*   DESCRIPTION
*
*       Get current log level.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       log_level_t    returns the current log level.
*
***********************************************************************/
log_level_t Log_Get_Level(void)
{
    return CURRENT_LOG_LEVEL;
}

/************************************************************************
*
*   FUNCTION
*
*       __Log_Write
*
*   DESCRIPTION
*
*       Write a va_list style payload to serial port
*
*   INPUTS
*
*       log_level_e    Log level for the current log
*       const chat*    format specifier
*       ...            variable argument list
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t __Log_Write(log_level_e level, const char *const fmt, ...)
{
    int32_t bytes_written = 0;

    /* Dump the log message over current log interface. */
    if (atomic_load_local_8(&Log_Interface) == LOG_DUMP_TO_TRACE)
    {
        va_list va;
        va_start(va, fmt);
        Trace_Format_String_V((trace_string_event_e)level, Trace_Get_MM_CB(), fmt, va);
        va_end(va);

        /* Evict trace buffer to L3 so that it can be access on host side for extraction
           through IOCTL */
        if (level <= LOG_MM_TRACE_EVICT_LEVEL)
        {
            Trace_Evict_Buffer_MM();
        }
    }
    else
    {
        char buff[LOG_STRING_MAX_SIZE_MM];
        va_list va;

        va_start(va, fmt);
        vsnprintf(buff, LOG_STRING_MAX_SIZE_MM, fmt, va);
        va_end(va);

        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        bytes_written = SERIAL_puts(PU_UART0, buff);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return bytes_written;
}

/************************************************************************
*
*   FUNCTION
*
*       __Log_Write_String
*
*   DESCRIPTION
*
*       Write a string of
*
*   INPUTS
*
*       log_level_e    Log level for the current log
*       char*          pointer to string
*       size_t         length of string
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t __Log_Write_String(log_level_e level, const char *str, size_t length)
{
    int32_t bytes_written = 0;

    /* Dump the log message over current log interface. */
    if (atomic_load_local_8(&Log_Interface) == LOG_DUMP_TO_TRACE)
    {
        Trace_String((trace_string_event_e)level, Trace_Get_MM_CB(), str);

        bytes_written = (int32_t)length;
        /* Evict trace buffer to L3 so that it can be access on host side for extraction
           through IOCTL */
        if (level <= LOG_MM_TRACE_EVICT_LEVEL)
        {
            Trace_Evict_Buffer_MM();
        }
    }
    else
    {
        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        bytes_written = SERIAL_write(PU_UART0, str, (int)length);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return bytes_written;
}
