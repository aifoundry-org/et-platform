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
*       __Log_Write
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
int32_t __Log_Write(log_level_e level, const char *const fmt, ...)
{
    /* TODO: Use Trace_Format_String() when Trace common library has support/alternative
       of libc_nano to process va_list string formatting. Also, remove log_level check
       from here, as trace library does that internally .*/
    va_list va;
    int32_t bytes_written;
    char buff[TRACE_STRING_MAX_SIZE_CM];
    va_start(va, fmt);
    bytes_written = vsnprintf(buff, TRACE_STRING_MAX_SIZE_CM, fmt, va);
    va_end(va);

    Trace_String(level, Trace_Get_CM_CB(), buff);

    /* Evict trace buffer to L3 so that it can be access on host side for extraction
        through IOCTL */
    if (level <= LOG_CM_TRACE_EVICT_LEVEL)
    {
        Trace_Evict_CM_Buffer();
    }

    return bytes_written;
}

/************************************************************************
*
*   FUNCTION
*
*       __Log_Write_Str
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
int32_t __Log_Write_Str(log_level_e level, const char *str, size_t length)
{
    Trace_String(level, Trace_Get_CM_CB(), str);

    /* Evict trace buffer to L3 so that it can be access on host side for extraction
           through IOCTL */
    if (level <= LOG_CM_TRACE_EVICT_LEVEL)
    {
        Trace_Evict_CM_Buffer();
    }

    return (int32_t)length;
}
