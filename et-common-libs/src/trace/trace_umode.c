/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file trace_umode.c
    \brief A C file that implements the Trace services for CM UMode.
*/
/***********************************************************************/

#include "etsoc/drivers/pmu/pmu.h"
#include "etsoc/isa/etsoc_memory.h"
#include "etsoc/isa/hart.h"
#include "etsoc/isa/syscall.h"

static inline uint64_t trace_umode_sample_sc_pmc(uint64_t pmc)
{
    /* Sample the pmc */
    return (uint64_t)syscall(SYSCALL_PMC_SC_SAMPLE, pmc, 0, 0);
}

static inline uint64_t trace_umode_sample_ms_pmc(uint64_t pmc)
{
    /* Sample the pmc */
    return (uint64_t)syscall(SYSCALL_PMC_MS_SAMPLE, pmc, 0, 0);
}

#define ET_TRACE_MEM_CPY(dest, src, size)    et_memcpy(dest, src, size)
#define ET_TRACE_STRLEN(str)                 et_strlen(str)
#define ET_TRACE_GET_TIMESTAMP()             PMC_Get_Current_Cycles()
#define ET_TRACE_GET_HPM_COUNTER(id)         pmu_core_counter_read_unpriv(id)
#define ET_TRACE_GET_SHIRE_CACHE_COUNTER(id) trace_umode_sample_sc_pmc(id)
#define ET_TRACE_GET_MEM_SHIRE_COUNTER(id)   trace_umode_sample_ms_pmc(id)
#define ET_TRACE_GET_HART_ID()               get_hart_id()
#define ET_TRACE_ENCODER_IMPL

#include "etsoc/common/utils.h"
#include "common/printf.h"
#include "trace/trace_umode.h"

#include <stdarg.h>
#include <stdio.h>

/************************************************************************
*
*   FUNCTION
*
*       __et_printf
*
*   DESCRIPTION
*
*       This function log the string message into Trace, If Trace was
*       enabled for caller Hart.
*       NOTE: CM UMode Trace is initialized by CM Runtime in SMode.
*
*   INPUTS
*
*       fmt     String format specifier
*       ...     Variable argument list
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void __et_printf(const char *fmt, ...)
{
    char data[TRACE_STRING_MAX_SIZE + 1];
    va_list va;
    va_start(va, fmt);

    vsnprintf(data, TRACE_STRING_MAX_SIZE, fmt, va);

    Trace_String(TRACE_EVENT_STRING_CRITICAL,
        &CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb, data);
}

/************************************************************************
*
*   FUNCTION
*
*       et_trace_flush_buffer
*
*   DESCRIPTION
*
*       This function evicts the Trace buffer of caller Worker Hart.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void et_trace_flush_buffer(void)
{
    const struct trace_control_block_t *cb = &CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb;

    if (cb->enable == TRACE_ENABLE)
    {
        if (cb->header == TRACE_STD_HEADER)
        {
            struct trace_buffer_std_header_t *trace_header =
                (struct trace_buffer_std_header_t *)cb->base_per_hart;

            trace_header->data_size = cb->offset_per_hart;
        }
        else
        {
            struct trace_buffer_size_header_t *size_header =
                (struct trace_buffer_size_header_t *)cb->base_per_hart;

            size_header->data_size = cb->offset_per_hart;
        }
    }

    /* Flush the buffer to L3. */
    ETSOC_MEM_EVICT((uint64_t *)cb->base_per_hart, cb->offset_per_hart, to_L3)
}
