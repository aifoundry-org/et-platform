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
/*! \file trace_umode.h
    \brief A C header that implements the Trace services for CM UMode.
*/
/***********************************************************************/

#ifndef TRACE_UMODE_H
#define TRACE_UMODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <et-trace/encoder.h>
#include "etsoc/isa/hart.h"

/*
 * Compute Minion UMode Trace control block.
 */
typedef struct umode_trace_control_block {
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) umode_trace_control_block_t;

/* NOTE: Keep it in sync with the memory map layout file in minion runtime. */
#define CM_UMODE_TRACE_CB_BASEADDR  0x8100d21040

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.
*/
#define GET_CB_INDEX(hart_id)       ((hart_id < 2048U) ? hart_id : (hart_id - 32U))

/*! \def CM_UMODE_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_UMODE_TRACE_CB           ((umode_trace_control_block_t*)CM_UMODE_TRACE_CB_BASEADDR)

/*! \def et_trace_pmc_compute()
    \brief A macro used to dump Minion and Neighborhood PMCs in trace buffer.
*/
#define et_trace_pmc_compute(hart_id)                                                      \
{                                                                                          \
    uint32_t cur_hart_id = get_hart_id();                                                  \
    if (hart_id == cur_hart_id)                                                            \
    {                                                                                      \
        Trace_PMC_Counters_Compute(&CM_UMODE_TRACE_CB[GET_CB_INDEX(cur_hart_id)].cb);      \
    }                                                                                      \
}

/*! \def et_trace_pmc_memory()
    \brief A macro used to dump Shire-Cache and Mem-Shire PMCs in trace buffer.
    \warning Must only we called from a single hart in a kernel.
*/
#define et_trace_pmc_memory(hart_id)                                                       \
{                                                                                          \
    uint32_t cur_hart_id = get_hart_id();                                                  \
    if (hart_id == cur_hart_id)                                                            \
    {                                                                                      \
        Trace_PMC_Counters_Memory(&CM_UMODE_TRACE_CB[GET_CB_INDEX(cur_hart_id)].cb);       \
    }                                                                                      \
}

#ifdef __cplusplus
}
#endif

#endif /* TRACE_UMODE_H */