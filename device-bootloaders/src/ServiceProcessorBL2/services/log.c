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
#include "log.h"
#include "trace.h"
#include "system/layout.h"
#include <stddef.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "dm_event_def.h"
#include "config/mgmt_build_config.h"

static void generate_runtime_error_event(uint32_t error_count);

/*
 * Log control block for current logging information.
 */
typedef struct log_cb
{
    union
    {
        struct
        {
            uint8_t current_log_level;
            uint8_t current_log_interface;
        };
        uint16_t raw_log_info;
    };
} log_cb_t;

/*! \var log_level_t Log_CB
    \brief Global variable that maintains the current log information
    \warning Not thread safe!
*/
static log_cb_t Log_CB __attribute__((aligned(64))) = { .current_log_level = LOG_LEVEL_WARNING,
                                                        .current_log_interface = LOG_DUMP_TO_UART };

static SemaphoreHandle_t Log_Mutex = NULL;
static StaticSemaphore_t Log_Mutex_Buffer;
static uint32_t RT_Error_Count = 0;

/*! \def CHECK_STRING_FILTER
    \brief This checks if trace string log level is enabled to log the given level.
*/
#define CHECK_STRING_FILTER(cb, log_level) \
    ((cb->filter_mask & TRACE_FILTER_STRING_MASK) >= log_level)

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
    Log_Mutex = xSemaphoreCreateMutexStatic(&Log_Mutex_Buffer);

    if (!Log_Mutex)
    {
        Log_Write(LOG_LEVEL_ERROR, "Log Mutex creation failed!\n");
        return;
    }

    /* Reset the error count */
    RT_Error_Count = 0;

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
    Log_CB.current_log_level = level;
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
    return Log_CB.current_log_level;
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
    Log_CB.current_log_interface = interface;
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
    return Log_CB.current_log_interface;
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
    int32_t bytes_written = 0;
    uint32_t error_count = 0;

    /* Check if given log level is enabled */
    if (level > Log_CB.current_log_level)
    {
        return bytes_written;
    }

    /* Dump the log message over current log interface. */
    if (Log_CB.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        char str[TRACE_STRING_MAX_SIZE_SP];
        va_list va;
        va_start(va, fmt);
        bytes_written = vsnprintf(str, TRACE_STRING_MAX_SIZE_SP, fmt, va);
        va_end(va);

        Trace_String(level, Trace_Get_SP_CB(), str);

        /* Update trace buffer header, this will update data size field in header
           to reflect current data in buffer. */
        Trace_Update_SP_Buffer_Header();

        /* Check if severity of message is error or above */
        if (level == LOG_LEVEL_ERROR)
        {
            /* Acquire the Mutex */
            if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
            {
                xSemaphoreTake(Log_Mutex, portMAX_DELAY);
            }

            /* Increment the error count */
            error_count = ++RT_Error_Count;

            /* Release the Mutex */
            if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
            {
                xSemaphoreGive(Log_Mutex);
            }
        }
    }
    else
    {
        /* Verify the logging level */
        if (level > Log_CB.current_log_level)
        {
            return 0;
        }

        char str[LOG_STRING_MAX_SIZE_SP];
        va_list va;
        va_start(va, fmt);
        bytes_written = vsnprintf(str, LOG_STRING_MAX_SIZE_SP, fmt, va);
        va_end(va);

        /* Acquire the Mutex */
        if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        {
            xSemaphoreTake(Log_Mutex, portMAX_DELAY);
        }

        /* Write the data to console */
        bytes_written = printf("%s", str);

        /* Check if severity of message is error or above */
        if (level == LOG_LEVEL_ERROR)
        {
            /* Increment the error count */
            error_count = ++RT_Error_Count;
        }

        /* Release the Mutex */
        if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        {
            xSemaphoreGive(Log_Mutex);
        }
    }

    /* Check if the error_count was updated. */
    if (error_count)
    {
        generate_runtime_error_event(error_count);
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
    uint32_t error_count = 0;

    /* Dump the log message over current log interface. */
    if (Log_CB.current_log_interface == LOG_DUMP_TO_TRACE)
    {
        Trace_String(level, Trace_Get_SP_CB(), str);

        /* Update trace buffer header, this will update data size field in header
           to reflect current data in buffer. */
        Trace_Update_SP_Buffer_Header();

        bytes_written = (int32_t)length;

        /* Check if severity of message is error or above */
        if (level == LOG_LEVEL_ERROR)
        {
            /* Acquire the Mutex */
            if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
            {
                xSemaphoreTake(Log_Mutex, portMAX_DELAY);
            }

            /* Increment the error count */
            error_count = ++RT_Error_Count;

            /* Release the Mutex */
            if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
            {
                xSemaphoreGive(Log_Mutex);
            }
        }
    }
    else
    {
        /* Verify the logging level */
        if (level > Log_CB.current_log_level)
        {
            return 0;
        }

        /* Acquire the Mutex */
        if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        {
            xSemaphoreTake(Log_Mutex, portMAX_DELAY);
        }

        /* Write the data to console */
        bytes_written = printf("%s", str);

        /* Check if severity of message is error or above */
        if (level == LOG_LEVEL_ERROR)
        {
            /* Increment the error count */
            error_count = ++RT_Error_Count;
        }

        /* Release the Mutex */
        if (Log_Mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        {
            xSemaphoreGive(Log_Mutex);
        }
    }

    /* Check if the error_count was updated. */
    if (error_count)
    {
        generate_runtime_error_event(error_count);
    }

    return bytes_written;
}

/************************************************************************
*
*   FUNCTION
*
*       generate_runtime_error_event
*
*   DESCRIPTION
*
*       This function sends an event to host when the runtime errors count
*       exceeds the predefined threshold.
*
*   INPUTS
*
*       error_count    Count for the number of errors
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
static void generate_runtime_error_event(uint32_t error_count)
{
    /* Check if the error threshold has reached */
    if (error_count > RT_ERROR_THRESHOLD)
    {
        /* Create an event message to notify the host */
        struct event_message_t message;

        /* Add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, SP_RUNTIME_ERROR, sizeof(struct event_message_t))

        /* Fill in the syndrome 2 with the error count above the threshold */
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, timer_get_ticks_count(),
                           error_count - RT_ERROR_THRESHOLD)

        /* Send the event message to the host */
        if (0 != SP_Host_Iface_CQ_Push_Cmd((void *)&message, sizeof(message)))
        {
            Log_Write(LOG_LEVEL_WARNING, "generate_runtime_error_event :  push to CQ failed!\n");
        }
    }
}
