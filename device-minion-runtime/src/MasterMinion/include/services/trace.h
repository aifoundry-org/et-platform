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
    \brief A C header that define the Trace services for Master Minion.
*/
/***********************************************************************/
#ifndef MM_TRACE_H
#define MM_TRACE_H

#include "device_trace.h"

/**************************/
/* MM Trace Status Codes  */
/**************************/
#define INVALID_TRACE_INIT_INFO       -10

/*! \fn void Trace_Init_MM(const struct trace_init_info_t *mm_init_info)
    \brief This function initializes Trace for all harts in Master Minion
           Shire
    \param mm_init_info Pointer Trace init information.
    \return None
*/
void Trace_Init_MM(const struct trace_init_info_t *mm_init_info);

/*! \fn struct trace_control_block_t* Trace_Get_MM_CB(uint64_t hart_id)
    \brief This function return Trace control block for given Hart ID.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t* Trace_Get_MM_CB(void);

#endif