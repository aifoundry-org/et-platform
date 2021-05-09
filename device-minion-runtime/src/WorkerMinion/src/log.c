#include "device-common/atomic.h"
#include "layout.h"
#include "log.h"
#include "printf.h"
#include "serial.h"
#include "sync.h"
#include "trace.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

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
        TODO: All worker harts currently crudely share the same Log CB.
*/
static log_cb_t Log_CB __attribute__((aligned(64))) = 
        {.current_log_level = LOG_LEVEL_WARNING, .current_log_interface = LOG_DUMP_TO_UART};

/************************************************************************
*
*   FUNCTION
*
*       log_set_level
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
void log_set_level(log_level_t level)
{
    atomic_store_global_8(&Log_CB.current_log_level, level);
}

/************************************************************************
*
*   FUNCTION
*
*       log_get_level
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
log_level_t log_get_level(void)
{
    return atomic_load_global_8(&Log_CB.current_log_level);
}

/************************************************************************
*
*   FUNCTION
*
*       log_set_interface
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
void log_set_interface(log_interface_t interface)
{
    atomic_store_global_8(&Log_CB.current_log_interface, interface);
}

/************************************************************************
*
*   FUNCTION
*
*       log_get_interface
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
log_interface_t log_get_interface(void)
{
    return atomic_load_global_8(&Log_CB.current_log_interface);
}

/************************************************************************
*
*   FUNCTION
*
*       log_write
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
*       int64_t        bytes written
*
***********************************************************************/
int64_t log_write(log_level_t level, const char *const fmt, ...)
{
    int ret=0;
    va_list va;
    char buff[128];
    log_cb_t log_cb;
    log_cb.raw_log_info = atomic_load_global_16(&Log_CB.raw_log_info);

    if (log_cb.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        /* TODO: Use Trace_Format_String() when Trace common library has support/alternative
           of libc_nano to process va_list string formatting. Also, remove log_level check 
           from here, as trace library does that internally .*/
        if(CHECK_STRING_FILTER(Trace_Get_CM_CB(), level))
        {
            va_start(va, fmt);
            vsnprintf(buff, sizeof(buff), fmt, va);

            Trace_String(level, Trace_Get_CM_CB(), buff);

            /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string 
            type message. */
            ret = TRACE_STRING_MAX_SIZE;
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
        ret = SERIAL_puts(UART0, buff);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return ret;
}

/************************************************************************
*
*   FUNCTION
*
*       log_write_str
*
*   DESCRIPTION
*
*       Write a string of given length.
*
*   INPUTS
*
*       log_level_t    log level
*       char*          pointer to string
*       size_t         length of string
*
*   OUTPUTS
*
*       int64_t        bytes written
*
***********************************************************************/
int64_t log_write_str(log_level_t level, const char *str, size_t length)
{
    int ret=0;
    log_cb_t log_cb;
    log_cb.raw_log_info = atomic_load_global_16(&Log_CB.raw_log_info);

    if (log_cb.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        Trace_String(level, Trace_Get_CM_CB(), str);

        /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string 
           type message. */
        ret = TRACE_STRING_MAX_SIZE;
    }
    else
    {
        if (level > log_cb.current_log_level)
        { 
            return 0;
        }

        acquire_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
        ret = SERIAL_write(UART0, str, (int)length);
        release_global_spinlock((spinlock_t *)FW_GLOBAL_UART_LOCK_ADDR);
    }

    return ret;
}
