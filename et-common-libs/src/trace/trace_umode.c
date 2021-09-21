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
/*! \file trace.h
    \brief A C file that implements the Trace services for CM UMode.
*/
/***********************************************************************/

#include <stddef.h>
#include <inttypes.h>
#include "etsoc/isa/hart.h"
#include <et-trace/layout.h>

#define ET_TRACE_ENCODER_IMPL
#define ET_TRACE_GET_HART_ID()  get_hart_id()
#include "etsoc/common/utils.h"
#include "common/printf.h"

/* NOTE: Keep it in sync with the memory map layout file in minion runtime. */
#define CM_UMODE_TRACE_CB_BASEADDR  0x8100d21040

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.
*/
#define GET_CB_INDEX(hart_id)       ((hart_id < 2048U)? hart_id: (hart_id - 32U))

/*! \def CM_UMODE_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_UMODE_TRACE_CB         ((umode_trace_control_block_t*)CM_UMODE_TRACE_CB_BASEADDR)

/*
 * Compute Minion UMode Trace control block.
 */
typedef struct umode_trace_control_block {
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) umode_trace_control_block_t;

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
    struct trace_control_block_t *cb = &CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb;
    char data[TRACE_STRING_MAX_SIZE + 1];
    va_list va;
    va_start(va, fmt);

    vsnprintf(data, TRACE_STRING_MAX_SIZE, fmt, va);

    Trace_String(TRACE_EVENT_STRING_CRITICAL, cb, data);
}
