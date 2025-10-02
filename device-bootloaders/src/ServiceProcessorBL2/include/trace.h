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
#include <et-trace/encoder.h>
#include <device-apis/device_apis_message_types.h>

/**************************/
/* SP Trace Status Codes  */
/**************************/
#define INVALID_TRACE_INIT_INFO -10
#define SP_TRACE_ENABLE         (1U << 0)
#define SP_TRACE_UART_ENABLE    (1U << 1)

/*! \def SP_TRACE_GET_ENTRY_OFFSET(addr)
    \brief Returns offset of addr from SP trace buffer base.
*/
#define SP_TRACE_GET_ENTRY_OFFSET(addr) ((uint64_t)addr - SP_TRACE_BUFFER_BASE)

/*! \fn int32_t Trace_Init_SP(const struct trace_init_info_t *sp_init_info)
    \brief This function initializes Trace for sp
    \param sp_init_info Pointer Trace init information.
    \return status of function call
*/
int32_t Trace_Init_SP(const struct trace_init_info_t *sp_init_info);

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

/*! \fn void Trace_Update_SP_Buffer_Header(void)
    \brief This function Updates Trace buffer header to reflect current data
           in buffer.
    \return None.
*/
void Trace_Update_SP_Buffer_Header(void);

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

/*! \fn Trace_Init_SP_Dev_Stats(const struct trace_init_info_t *dev_trace_init_info)
    \brief This function initializes Trace buffer for device stats.
    \param init_info Pointer Trace init information.
    \return None.
*/
int32_t Trace_Init_SP_Dev_Stats(const struct trace_init_info_t *dev_trace_init_info);

/*! \fn struct trace_control_block_t* Trace_Get_Dev_Stats_CB(void)
    \brief This function returns Trace control block for dev stats buffer.
    \return Pointer to the Trace control block .
*/
struct trace_control_block_t *Trace_Get_Dev_Stats_CB(void);

/*! \fn void Trace_Update_SP_Stats_Buffer_Header(void)
    \brief This function Updates stats Trace buffer header to reflect current data
           in buffer.
    \return None.
*/
void Trace_Update_SP_Stats_Buffer_Header(void);

/*! \fn Trace_Run_Control_SP_Dev_Stats(trace_enable_e control)
    \brief This function enable/disable Trace for Service Processor stats buffer.
    \param control Enable/Disable Trace.
    \return None.
*/
void Trace_Run_Control_SP_Dev_Stats(trace_enable_e control);

/*! \fn Trace_Reset_SP_Dev_Stats_Buffer(void)
    \brief This function resets stats trace buffer
    \return None.
*/
void Trace_Reset_SP_Dev_Stats_Buffer(void);

#endif
