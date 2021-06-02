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
/*! \file trace.c
    \brief A C module that implements the Trace services

    Public interfaces:
        Trace_Init_SP
        Trace_Get_SP_CB

*/
/***********************************************************************/

#include "log.h"
#include "trace.h"
#include <string.h>
#include "config/mgmt_build_config.h"

/*
 * Service Processor Trace control block.
 */
static struct trace_control_block_t SP_Trace_CB;


/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_SP
*
*   DESCRIPTION
*
*       This function initializes Trace for Service Processor
*
*   INPUTS
*
*       trace_init_info_t    Trace init info for Service Processor .
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Init_SP(const struct trace_init_info_t *sp_init_info)
{
    struct trace_init_info_t sp_init_info_l;

    /* If init information is NULL then do default initialization. */
    if (sp_init_info == NULL)
    {
        /* Populate default Trace configurations for Service Processor. */
        sp_init_info_l.buffer        = SP_TRACE_BUFFER_BASE;
        sp_init_info_l.buffer_size   = SP_TRACE_BUFFER_SIZE;
        sp_init_info_l.event_mask    = TRACE_EVENT_STRING;
        sp_init_info_l.filter_mask   = TRACE_EVENT_STRING_WARNING;
        sp_init_info_l.threshold     = SP_TRACE_BUFFER_SIZE;
    }
    else
    {
        memcpy(&sp_init_info_l, sp_init_info, sizeof(struct trace_init_info_t));
    }

    /* Common buffer for all SP HART. */
    SP_Trace_CB.size_per_hart = SP_TRACE_BUFFER_SIZE;
    SP_Trace_CB.base_per_hart = SP_TRACE_BUFFER_BASE;

    /* Initialize Trace for each all Harts in Service Processor. */
    Trace_Init(&sp_init_info_l, &SP_Trace_CB);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_SP_CB
*
*   DESCRIPTION
*
*       This function returns the common Trace control block (CB) for all
*       SP Harts.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       trace_control_block_t Pointer to the Trace control block.
*
***********************************************************************/
struct trace_control_block_t* Trace_Get_SP_CB(void)
{
    return &SP_Trace_CB;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_RT_Control_SP
*
*   DESCRIPTION
*
*       This function enable/disable Trace for Service Processor.
*
*   INPUTS
*
*       enum trace_enable_e    Enable/Disable Trace.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_RT_Control_SP(enum trace_enable_e state)
{
    SP_Trace_CB.enable = state;
}