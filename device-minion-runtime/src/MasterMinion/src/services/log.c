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
        __Log_Write
        __Log_Write_String
*/
/***********************************************************************/
#include "services/log.h"
#include "services/trace.h"
#include "device-common/hart.h"
#include "drivers/console.h"
#include "device-common/atomic.h"
#include "layout.h"
#include <stddef.h>
#include "sync.h"

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
    init_global_spinlock((spinlock_t*)FW_GLOBAL_UART_LOCK_ADDR, 0);
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
*       __Log_Write
*
*   DESCRIPTION
*
*       Write a va_list style payload to serial port
*
*   INPUTS
*
*       const chat*    format specifier
*       ...            variable argument list
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t __Log_Write(const char *const fmt, ...)
{
    char buff[128];
    va_list va;
    int32_t bytes_written = 0;

    /* Dump the log message over current log interface. */
    if (atomic_load_local_8(&Log_Interface) == LOG_DUMP_TO_TRACE)
    {
        /* TODO: Use Trace_Format_String() when Trace common library has support/alternative
           of libc_nano to process va_list string formatting. Also, remove log_level check 
           from here, as trace library does that internally .*/
        va_start(va, fmt);
        vsnprintf(buff, sizeof(buff), fmt, va);

        Trace_String((enum trace_string_event)CURRENT_LOG_LEVEL, Trace_Get_MM_CB(), buff);

        /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string
        type message. */
        bytes_written = TRACE_STRING_MAX_SIZE;
    }
    else
    {
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
*       __Log_Write_String
*
*   DESCRIPTION
*
*       Write a string of
*
*   INPUTS
*
*       char*          pointer to string
*       size_t         length of string
*
*   OUTPUTS
*
*       int32_t        bytes written
*
***********************************************************************/
int32_t __Log_Write_String(const char *str, size_t length)
{
    int32_t bytes_written = 0;

    /* Dump the log message over current log interface. */
    if (atomic_load_local_8(&Log_Interface) == LOG_DUMP_TO_TRACE)
    {
        Trace_String((enum trace_string_event)CURRENT_LOG_LEVEL, Trace_Get_MM_CB(), str);

        /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string 
           type message. */
        bytes_written = TRACE_STRING_MAX_SIZE;
    }
    else
    {
        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        bytes_written = SERIAL_write(UART0, str, (int)length);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return bytes_written;
}
