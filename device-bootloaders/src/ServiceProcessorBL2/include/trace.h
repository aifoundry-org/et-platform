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
    \brief A C header that define the Trace services for Service Processor.
*/
/***********************************************************************/
#ifndef SP_TRACE_H
#define SP_TRACE_H

#include "dm.h"
#include "dm_event_def.h"
#include <et-trace/encoder.h>
#include "sp_host_iface.h"
#include "log.h"

/**************************/
/* SP Trace Status Codes  */
/**************************/
#define INVALID_TRACE_INIT_INFO -10
#define SP_TRACE_ENABLE         (1U << 0)
#define SP_TRACE_UART_ENABLE    (1U << 1)

/*
 * SP Trace custom event IDs
*/
enum {
    SP_TRACE_CUSTOM_ID_START = 0,
    SP_TRACE_CUSTOM_ID_PERF_GLOBALS,
    SP_TRACE_CUSTOM_ID_POWER_GLOBALS,
    SP_TRACE_CUSTOM_ID_POWER_STATES_GLOBALS,
    SP_TRACE_CUSTOM_ID_COUNT
} sp_trace_custom_id;

/*! \def SP_TRACE_GET_ENTRY_OFFSET(addr)
    \brief Returns offset of addr from SP trace buffer base.
*/
#define SP_TRACE_GET_ENTRY_OFFSET(addr) ((uint64_t)addr - SP_TRACE_BUFFER_BASE)

/*! \fn void Trace_Init_SP(const struct trace_init_info_t *sp_init_info)
    \brief This function initializes Trace for sp
    \param sp_init_info Pointer Trace init information.
    \return None
*/
void Trace_Init_SP(const struct trace_init_info_t *sp_init_info);

/*! \fn struct trace_control_block_t* Trace_Get_SP_CB(uint64_t hart_id)
    \brief This function return Trace control block for given Hart ID.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t *Trace_Get_SP_CB(void);

/*! \fn uint32_t Trace_Get_SP_Buffer(void)
    \brief This function returns the common Trace control block buffer offset per
*       SP Hart.
    \return value of buffer offset.
*/
uint32_t Trace_Get_SP_Buffer(void);

/*! \fn void Trace_RT_Process_CMD(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
    \brief This function processes trace commands from host, this includes
            configuring trace for Service Processor.
    \return None.
*/
void Trace_Process_CMD(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

/*! \fn Trace_Process_Control_Cmd(void *buffer)
    \brief This function processes trace commands from host, to enable/disable
            the  trace for Service Processor.
    \return None.
*/
void Trace_Process_Control_Cmd(void *buffer);

/*! \fn Trace_Process_Config_Cmd(void *buffer)
    \brief This function processes trace commands from host, to specify Event and
            filter mask the  trace for Service Processor.
    \return None.
*/
void Trace_Process_Config_Cmd(void *buffer);

/*! \fn Trace_Exception_Init_SP(const struct trace_init_info_t *sp_init_info)
    \brief This function initializes Trace buffer for SP exception.
    \param init_info Pointer Trace init information.
    \return None.
*/
int32_t Trace_Exception_Init_SP(const struct trace_init_info_t *init_info);

/*! \fn struct trace_control_block_t* Trace_Get_SP_Exp_CB(void)
    \brief This function returns Trace control block for Exception buffer.
    \return Pointer to the Trace control block .
*/
struct trace_control_block_t *Trace_Get_SP_Exp_CB(void);

/*! \fn Trace_Exception_Dump_Context(const void *stack_frame)
    \brief This function dumps stack context to trace and returns
          trace buffer offset.
    \param stack_frame current stack frame context including GPRs and CSRS
    \return pointer to trace buffer with dumped stack context.
*/
uint8_t *Trace_Exception_Dump_Context(const void *stack_frame);

#endif
