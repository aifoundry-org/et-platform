/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file cm_to_mm_iface.h
    \brief A C header that define the MM to CM Interface.
*/
/***********************************************************************/
#ifndef CM_TO_MM_IFACE_H
#define CM_TO_MM_IFACE_H

#include "cm_mm_defines.h"
#include "mm_to_cm_iface.h"
#include "transports/mm_cm_iface/message_types.h"

/*! \fn int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx,
    const cm_iface_message_t *const message)
    \brief Function to send any unicast message from CM to MM.
    \param ms_thread_id Index of the thread in Master shire
    \param cb_idx Index of the unicast buffer
    \param message Pointer to message buffer
    \return Status success or error
    \warning Not thread safe. Only one caller per cb_idx.
    User needs to use the respective locking APIs if thread safety is required.
*/
int8_t CM_To_MM_Iface_Unicast_Send(
    uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message);

/*! \fn int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer,
    uint64_t type, uint64_t hart_id, const internal_execution_context_t *context)
    \brief Function to save execution context in a buffer
    \param context_buffer Pointer to context buffer
    \param type Type of exception
    \param hart_id Hart ID of thread that took exception
    \param context Context of exception
    \return success or error
*/
int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer, uint64_t type,
    uint64_t hart_id, const internal_execution_context_t *context);

/*! \fn int8_t CM_To_MM_Save_Kernel_Error(execution_context_t *context_buffer, uint64_t hart_id, uint64_t error_type,
    int64_t error_code)
    \brief Function to save kernel execution error in a buffer
    \param context_buffer Pointer to context buffer
    \param hart_id Hart ID of thread that took error
    \param error_type Type of error
    \param error_code Error code to be saved
    \return Status success or error
*/
int8_t CM_To_MM_Save_Kernel_Error(
    execution_context_t *context_buffer, uint64_t hart_id, uint64_t error_type, int64_t error_code);

#endif
