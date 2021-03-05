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


#endif
