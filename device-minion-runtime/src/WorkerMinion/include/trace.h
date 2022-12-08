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

#include <et-trace/encoder.h>

/*! \fn void Trace_Init_CM(const struct trace_init_info_t *cm_init_info)
    \brief This function initializes Trace for a single Hart in CM Shires.
           All CM Harts must call this function to Enable Trace.
    \param cm_init_info Pointer Trace init information.
    \return Successful status or error code.
*/
int32_t Trace_Init_CM(const struct trace_init_info_t *cm_init_info);

/*! \fn int32_t Trace_Configure_CM(const struct trace_config_info_t *cm_config_info)
    \brief This function configures the Trace.
           NOTE:Trace must be initialized using Trace_Init_CM() before this function.
    \param cm_config_info Pointer to Trace config info.
    \return Successful status or error code.
*/
int32_t Trace_Configure_CM(const struct trace_config_info_t *cm_config_info);

/*! \fn struct trace_control_block_t* Trace_Get_CM_CB(void)
    \brief This function returns the Trace control block (CB) of
           the Worker Hart which is calling this function.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t *Trace_Get_CM_CB(void);

/*! \fn svoid Trace_Evict_CM_Buffer(void)
    \brief This function evicts the Trace buffer of caller Worker Hart.
    \return None
*/
void Trace_Evict_CM_Buffer(void);

/*! \fn void Trace_RT_Control_CM(uint32_t control)
    \brief This function updates the control of Trace for Compute Minnion
          runtime.
    \param control Bit encoded trace control flags.
    \return None
*/
void Trace_RT_Control_CM(uint32_t control);

/*! \fn void Trace_Set_Enable_CM(trace_enable_e enable)
    \brief This function enables/disables Trace for Compute Minnion.
    \param control Enum to Enable/Disable Trace
    \return None
*/
void Trace_Set_Enable_CM(trace_enable_e control);

/*! \fn void Trace_Init_UMode(const struct trace_init_info_t *init_info)
    \brief This function initializes Trace for a single Compute Hart.
           All Harts can call this function to Enable/Dsiable its Trace.
           Shire and Thread decides if Trace need to be enabled or disabled for caller Hart.
    \param init_info Pointer to Trace init information.
    \return Success or Error code.
*/
void Trace_Init_UMode(const struct trace_init_info_t *init_info);

/*! \fn void Trace_Update_UMode_Buffer_Header(void)
    \brief Update buffer header to reflect data size in buffer
    \return None
*/
void Trace_Update_UMode_Buffer_Header(void);

/*! \fn void Trace_Evict_UMode_Buffer(void)
    \brief This function evicts the Trace buffer of caller Worker Hart.
    \return None
*/
void Trace_Evict_UMode_Buffer(void);

#endif
