#ifndef __BL2_EXCEPTION__
#define __BL2_EXCEPTION__
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
/*! \file bl2_exception.h
    \brief A header file providing the exception handler interface

    Public interfaces:
    bl2_exception_entry
    bl2_dump_stack_frame
*/
/***********************************************************************/
#include "dm.h"
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"

/*! \fn void bl2_exception_entry(stack_frame)
    \brief High level exception handler - dumps the system state to trace buffer or console
            in case of exceptions.
    \param none
    \returns none
*/
void bl2_exception_entry(void *stack_frame);

/*! \fn void bl2_exception_entry(stack_frame)
    \brief Dumps system stack frame from ISR context
    \param none
    \returns none
*/
void bl2_dump_stack_frame(void);

#endif