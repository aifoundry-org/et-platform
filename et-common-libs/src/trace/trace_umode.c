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

#include "common/printf.h"

static inline uint64_t trace_umode_sample_sc_pmc(uint64_t pmc)
{
    /* Sample the pmc */
    return (uint64_t)syscall(SYSCALL_PMC_SC_SAMPLE, get_shire_id(), get_neighborhood_id(), pmc);
}

static inline uint64_t trace_umode_sample_ms_pmc(uint64_t pmc, uint8_t ms_id)
{
    /* Sample the pmc */
    return (uint64_t)syscall(SYSCALL_PMC_MS_SAMPLE, ms_id, pmc, 0);
}

/* Internal externs */
extern void *et_memcpy(void *dest, const void *src, size_t n);
extern size_t et_strlen(const char *str);

#define ET_TRACE_READ_MEM(dest, src, size)            et_memcpy(dest, src, size)
#define ET_TRACE_WRITE_MEM(dest, src, size)           et_memcpy(dest, src, size)
#define ET_TRACE_STRLEN(str)                          et_strlen(str)
#define ET_TRACE_VSNPRINTF(buffer, count, format, va) vsnprintf(buffer, count, format, va)
#define ET_TRACE_GET_TIMESTAMP()                      PMC_Get_Current_Cycles()
#define ET_TRACE_GET_TIMESTAMP_SAFE()                 PMC_Get_Current_Cycles_Safe()
#define ET_TRACE_GET_HPM_COUNTER(id)                  pmu_core_counter_read_unpriv(id)
#define ET_TRACE_GET_HPM_COUNTER_SAFE(id)             pmu_core_counter_read_unpriv_safe(id)
#define ET_TRACE_GET_SHIRE_CACHE_COUNTER(id)          trace_umode_sample_sc_pmc(id)
#define ET_TRACE_GET_MSHIRE_COUNTER(id, ms_id)        trace_umode_sample_ms_pmc(id, ms_id)
#define ET_TRACE_GET_HART_ID()                        get_hart_id()
#define ET_TRACE_STRING_MAX_SIZE                      128

#define ET_TRACE_ENCODER_IMPL

#include "trace/trace_umode.h"

#include <stdarg.h>
#include <stdio.h>

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

        /* Flush the buffer to L3. */
        ETSOC_MEM_EVICT((uint64_t *)cb->base_per_hart, cb->offset_per_hart, to_L3)
    }
}
