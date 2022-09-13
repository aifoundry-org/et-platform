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

/* mm_rt_helpers */
#include "cm_mm_defines.h"

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

/*! \def TRACE_CONFIG_CHECK_MM_HART
    \brief Helper macro to check if given shire and thread masks contains any MM HART.
*/
#define TRACE_CONFIG_CHECK_MM_HART(shire_mask, thread_mask) \
    (shire_mask & MM_SHIRE_MASK) && (thread_mask & MM_HART_MASK)

/*! \def TRACE_CONFIG_CHECK_CM_HART
    \brief Helper macro to check if given shire and thread masks contains any CM HART.
*/
#define TRACE_CONFIG_CHECK_CM_HART(shire_mask, thread_mask)            \
    (((shire_mask & CM_SHIRE_MASK) && (thread_mask & CM_HART_MASK)) || \
        ((shire_mask & MM_SHIRE_MASK) && (thread_mask & CW_IN_MM_SHIRE)))

/*! \def TRACE_STRING_MAX_SIZE_MM
    \brief Max string message lentgh which can be logged into Trace.
        NOTE: This will be removed as a result of SW-13550. Because
        it will direclty use Trace encoder for string formatting.
*/
#define TRACE_STRING_MAX_SIZE_MM 128

/*! \fn int32_t Trace_Init_MM(const struct trace_init_info_t *mm_init_info)
    \brief This function initializes Trace for all harts in Master Minion
           Shire
    \param mm_init_info Pointer Trace init information.
    \return Successful status or error code.
*/
int32_t Trace_Init_MM(const struct trace_init_info_t *mm_init_info);

/*! \fn int32_t Trace_Configure_MM(const struct trace_config_info_t *mm_config_info)
    \brief This function configures the Matser Minion Trace.
        NOTE:Trace must be initialized using Trace_Init_MM() before this function.
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

/*! \fn uint32_t Trace_Evict_Buffer_MM()
    \brief  This function Evict the MM Trace buffer upto current used buffer,
            it also updates the trace buffer header to include buffer usage.
    \return Size of buffer that was used and victed.
*/
uint32_t Trace_Evict_Buffer_MM(void);

/*! \fn uint32_t Trace_Evict_Buffer_MM_Stats(uint8_t trace_buf_type)
    \brief  This function Evict the MM Stats Trace buffer upto current used buffer,
            it also updates the trace buffer header to include buffer usage.
    \return Size of buffer that was used and victed.
*/
uint32_t Trace_Evict_Buffer_MM_Stats(void);

/*! \fn void Trace_RT_Control_MM_Stats(uint32_t control)
    \brief This function updates control of MM Trace runtime.
    \param control Bit encoded trace control flags.
    \return None
*/
void Trace_RT_Control_MM_Stats(uint32_t control);

/*! \fn uint32_t Trace_Evict_Event_MM_Stats(void * entry, uint32_t size)
    \brief This function evicts single MM Stat event packet, it also
           updates the trace buffer header to include buffer usage.
    WARNING: This function must be called everytime an event is
           logged into MM Stat Trace. Otherwise it is not safe.
    \param entry Starting address of event.
    \param size Size of event.
    \return Size of buffer that was used and victed.
*/
uint32_t Trace_Evict_Event_MM_Stats(const void *entry, uint32_t size);

/*! \fn int32_t Trace_Init_MM_Stats(const struct trace_init_info_t *mm_init_info)
    \brief This function initializes Stats trace for Master Minion
           Shire
    \param mm_init_info Pointer Trace init information.
    \return Successful status or error code.
*/
int32_t Trace_Init_MM_Stats(const struct trace_init_info_t *mm_init_info);

/*! \fn struct trace_control_block_t* Trace_Get_MM_Stats_CB(uint64_t hart_id)
    \brief This function return Trace control block for mm dev stats.
    \return Pointer to the Trace control block for caller Hart.
*/
struct trace_control_block_t *Trace_Get_MM_Stats_CB(void);

#endif
