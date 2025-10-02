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
*/
/***********************************************************************/
#include "dm.h"
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"
#include "trace.h"
#include "dm_event_control.h"

#define SP_EXCEPTION_STACK_FRAME_SIZE (sizeof(uint64_t) * 28)
#define SP_PERF_GLOBALS_SIZE                                                                       \
    (sizeof(struct asic_frequencies_t) + sizeof(struct dram_bw_t) + sizeof(struct max_dram_bw_t) + \
     sizeof(uint32_t) + sizeof(uint64_t))
#define SP_POWER_STATES_GLOBALS_SIZE (sizeof(struct residency_t) * 9)
#define SP_POWER_GLOBALS_SIZE                                           \
    (sizeof(power_state_e) + (sizeof(uint8_t) * 3) + sizeof(uint16_t) + \
     sizeof(struct module_uptime_t) + sizeof(struct module_voltage_t))
#define SP_GLOBALS_SIZE \
    (SP_PERF_GLOBALS_SIZE + SP_POWER_STATES_GLOBALS_SIZE + SP_POWER_GLOBALS_SIZE)

/*! \fn void bl2_exception_entry(stack_frame)
    \brief High level exception handler - dumps the system state to trace buffer or console
            in case of exceptions.
    \param none
    \returns none
*/
void bl2_exception_entry(const void *stack_frame);

/*! \fn void BL2_Report_Event(uint64_t trace_buffer_offset)
    \brief Send event to host on priority
    \param trace_buffer_offset trace buffer offset
    \param event event id to report
    \returns none
*/
void BL2_Report_Event(uint64_t trace_buffer_offset, enum event_ids event);

#endif
