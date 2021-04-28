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
    \brief A C header that define the Trace services for Compute Minions.
*/
/***********************************************************************/
#ifndef CM_TRACE_H
#define CM_TRACE_H

#include "device_trace.h"

/*! \fn void Trace_Init_CM(const struct trace_init_info_t *cm_init_info)
    \brief This function initializes Trace for a single Hart in CM Shires.
           All CM Harts must call this function to Enable Trace.
    \param cm_init_info Pointer Trace init information.
    \return None
*/
void Trace_Init_CM(const struct trace_init_info_t *cm_init_info);

/*! \fn struct trace_control_block_t* Trace_Get_CM_CB(void)
    \brief This function returns the Trace control block (CB) of
           the Worker Hart which is calling this function.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t* Trace_Get_CM_CB(void);

/*! \fn svoid Trace_Evict_CM_Buffer(void)
    \brief This function evicts the Trace buffer of caller Worker Hart.
    \return None
*/
void Trace_Evict_CM_Buffer(void);

/*! \fn void Trace_RT_Control_CM(enum trace_enable_e enable)
    \brief This function updates the control of Trace for Computer Minnion
          runtime.
    \param trace_enable_e Enable / Disbale Trace.
    \return None
*/
void Trace_RT_Control_CM(enum trace_enable_e enable);

#endif