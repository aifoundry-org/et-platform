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
/*! \file trace_umode_cb.h
    \brief A C header that contains the defines and data structures
    related to trace U-mode control block.
*/
/***********************************************************************/

#ifndef TRACE_UMODE_CB_H
#define TRACE_UMODE_CB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <et-trace/encoder.h>

/*
 * Compute Minion UMode Trace control block.
 */
typedef struct umode_trace_control_block {
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) umode_trace_control_block_t;

/* WARNING: Keep it in sync with the memory map layout file in minion runtime. */
#define CM_UMODE_TRACE_CB_BASEADDR  0x8004d22000ULL

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.
*/
#define GET_CB_INDEX(hart_id)       ((hart_id < 2048U) ? hart_id : (hart_id - 32U))

/*! \def CM_UMODE_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_UMODE_TRACE_CB           ((umode_trace_control_block_t*)CM_UMODE_TRACE_CB_BASEADDR)

#ifdef __cplusplus
}
#endif

#endif /* TRACE_UMODE_CB_H */