#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "log.h"
#include "trace.h"

#include <etsoc/isa/atomic.h>
#include <common/printf.h>
#include <etsoc/drivers/serial/serial.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>

/*! \def CHECK_STRING_FILTER
    \brief This checks if trace string log level is enabled to log the given level.
*/
#define CHECK_STRING_FILTER(cb, log_level) \
    ((cb->filter_mask & TRACE_FILTER_STRING_MASK) >= log_level)

/************************************************************************
*
*   FUNCTION
*
*       log_write
*
*   DESCRIPTION
*
*       Write a log with va_list style args
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
int32_t __log_write(log_level_e level, const char *const fmt, ...)
{
    /* TODO: Use Trace_Format_String() when Trace common library has support/alternative
       of libc_nano to process va_list string formatting. Also, remove log_level check
       from here, as trace library does that internally .*/
    va_list va;
    char buff[128];
    va_start(va, fmt);
    vsnprintf(buff, sizeof(buff), fmt, va);

    Trace_String(level, Trace_Get_CM_CB(), buff);

    /* Evict trace buffer to L3 so that it can be access on host side for extraction
        through IOCTL */
    if (level <= LOG_CM_TRACE_EVICT_LEVEL)
    {
        Trace_Evict_CM_Buffer();
    }

    /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string
        type message. */
    return TRACE_STRING_MAX_SIZE;
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
int32_t __log_write_str(log_level_e level, const char *str, size_t length)
{
    (void)length;

    Trace_String(level, Trace_Get_CM_CB(), str);

    /* Evict trace buffer to L3 so that it can be access on host side for extraction
           through IOCTL */
    if (level <= LOG_CM_TRACE_EVICT_LEVEL)
    {
        Trace_Evict_CM_Buffer();
    }
    /* Trace always consumes TRACE_STRING_MAX_SIZE bytes for every string
        type message. */
    return TRACE_STRING_MAX_SIZE;
}
