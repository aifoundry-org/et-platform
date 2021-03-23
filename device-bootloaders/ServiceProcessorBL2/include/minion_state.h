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
/*! \file minion_state.h
    \brief A C header that defines the Minion State service's
    public interfaces. These interfaces provide services using which
    the host can query the minion/thread states.
*/
/***********************************************************************/
#ifndef __MINION_STATE_H__
#define __MINION_STATE_H__

#include "dm.h"
#include "mm_sp_cmd_spec.h"
#include "sp_host_iface.h"
#include "sp_mm_iface.h"
#include "mm_sp_cmd_spec.h"
#include "dm_event_def.h"

/*!
 * @enum minion_error_type
 * @brief Enum defining event/error type
 */
enum minion_error_type {
    EXCEPTION,
    HANG,
};

/*! \brief Initialize Minion state service
    \param active_shire_mask Mask of active Shires
    \returns none
*/
void Minion_State_Init(uint64_t active_shire_mask);

/*! \brief Process the Minion State related host request
    \param tag_id Tag ID
    \param msg_id Unique enum representing specific command
    \returns none
*/
void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id);

/*! \brief Process the Minion State related MM request
    \param msg_id Unique enum representing specific command
    \returns none
*/
void Minion_State_MM_Iface_Process_Request(uint8_t msg_id);

/*! \fn int32_t minion_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the Minion error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/

int32_t minion_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t minion_error_control_deinit(void)
    \brief This function cleans up the minion error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t minion_error_control_deinit(void);

/*! \fn int32_t minion_set_except_threshold(uint32_t ce_threshold)
    \brief This function programs the minion exception error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t minion_set_except_threshold(uint32_t ce_threshold);

/*! \fn int32_t minion_set_hang_threshold(uint32_t ce_threshold)
    \brief This function programs the minion hang error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t minion_set_hang_threshold(uint32_t ce_threshold);

/*! \fn int32_t minion_get_except_err_count(uint32_t *err_count)
    \brief This function returns the minion exception error count
    \param err_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t minion_get_except_err_count(uint32_t *err_count);

/*! \fn int32_t minion_get_hang_err_count(uint32_t *err_count)
    \brief This function returns the minion hang error count
    \param err_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t minion_get_hang_err_count(uint32_t *err_count);

/*! \fn void minion_error_update_count(uint8_t error_type)
    \brief This function rupdates error count depending upone
            error type either EXCEPTION or HANG
    \param error_type error type to update counter for
    \return nothing
*/
void minion_error_update_count(uint8_t error_type);

#endif
