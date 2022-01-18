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

/* et_trace */
#include <et-trace/encoder.h>

/* mm_rt_svcs */
#include <transports/mm_cm_iface/message_types.h>

/* Internal headers */
#include "config/mm_config.h"

#ifdef MM_ENABLE_CMD_EXECUTION_TRACE
#define TRACE_LOG_CMD_STATUS(message_id, sqw_idx, tag_id, status)              \
    {                                                                          \
        struct trace_event_cmd_status_t cmd_data = { .queue_slot_id = sqw_idx, \
            .mesg_id = message_id,                                             \
            .trans_id = tag_id,                                                \
            .cmd_status = status };                                            \
        Trace_Cmd_Status(Trace_Get_MM_CB(), &cmd_data);                        \
    }
#else
#define TRACE_LOG_CMD_STATUS(message_id, sqw_idx, tag_id, status)
#endif

/*! \def CM_SHIRE_MASK
    \brief Shire mask of Compute Workers.
*/
#define CM_SHIRE_MASK 0x1FFFFFFFFULL

/*! \def CW_IN_MM_SHIRE
    \brief Computer worker HART index in MM Shire.
*/
#define CW_IN_MM_SHIRE 0xFFFFFFFF00000000ULL

/*! \def TRACE_CONFIG_CHECK_MM_HART
    \brief Helper macro to check if given shire and thread masks contains any MM HART.
*/
#define TRACE_CONFIG_CHECK_MM_HART(shire_mask, thread_mask) \
    (shire_mask & MM_SHIRE_MASK) && (thread_mask & MM_HART_MASK)

/*! \def TRACE_CONFIG_CHECK_CM_HART
    \brief Helper macro to check if given shire and thread masks contains any CM HART.
*/
#define TRACE_CONFIG_CHECK_CM_HART(shire_mask, thread_mask)            \
    (((shire_mask & CM_SHIRE_MASK) && (thread_mask & MM_HART_MASK)) || \
        ((shire_mask & MM_SHIRE_MASK) && (thread_mask & CW_IN_MM_SHIRE)))

/*! \fn int32_t Trace_Init_MM(const struct trace_init_info_t *mm_init_info)
    \brief This function initializes Trace for all harts in Master Minion
           Shire
    \param mm_init_info Pointer Trace init information.
    \return Successful status or error code.
*/
int32_t Trace_Init_MM(const struct trace_init_info_t *mm_init_info);

/*! \fn int32_t Trace_Configure_MM(const struct trace_config_info_t *mm_config_info)
    \brief This function configures the Matser Minion Trace.
*       NOTE:Trace must be initialized using Trace_Init_MM() before this function.
    \param mm_config_info Pointer Trace init information.
    \return Successful status or error code.
*/
int32_t Trace_Configure_MM(const struct trace_config_info_t *mm_config_info);

/*! \fn struct trace_control_block_t* Trace_Get_MM_CB(uint64_t hart_id)
    \brief This function return Trace control block for given Hart ID.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t *Trace_Get_MM_CB(void);

/*! \fn uint64_t Trace_Get_CM_Shire_Mask(void)
    \brief This function returns shire mask of Compute Minions for which
           Trace is enabled.
    \return CM Shire Mask.
*/
uint64_t Trace_Get_CM_Shire_Mask(void);

/*! \fn uint64_t Trace_Get_CM_Thread_Mask(void)
    \brief This function returns Thread mask of Compute Minions for which
           Trace is enabled.
    \return CM Thread Mask.
*/
uint64_t Trace_Get_CM_Thread_Mask(void);

/*! \fn int32_t Trace_Configure_CM_RT(mm_to_cm_message_trace_rt_config_t *config_msg)
    \brief This function configures CM RT tracing
    \param config_msg Pointer to CM config message info
    \return Success or failure.
*/
int32_t Trace_Configure_CM_RT(mm_to_cm_message_trace_rt_config_t *config_msg);

/*! \fn void Trace_RT_Control_MM(uint32_t control)
    \brief This function updates control of MM Trace runtime.
    \param control Bit encoded trace control flags.
    \return None
*/
void Trace_RT_Control_MM(uint32_t control);

/*! \fn uint32_t Trace_Evict_Buffer_MM(void)
    \brief  This function Evict the MM Trace buffer upto current used buffer,
            it also updates the trace buffer header to include buffer usage.
    \return Size of buffer that was used and victed.
*/
uint32_t Trace_Evict_Buffer_MM(void);

/*! \fn void Trace_Set_Enable_MM(trace_enable_e enable)
    \brief This function enables/disables Trace for Master Minnion.
    \param control Enum to Enable/Disable Trace
    \return None
*/
void Trace_Set_Enable_MM(trace_enable_e control);

#endif
