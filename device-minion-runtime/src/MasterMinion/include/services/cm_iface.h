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

/***********************************************************************/
/*! \file cm_iface.h
    \brief A C header that defines the MM to CM public interfaces.
*/
/***********************************************************************/
#ifndef CM_IFACE_H
#define CM_IFACE_H

/* mm_rt_svcs */
#include "transports/mm_cm_iface/message_types.h"

/* mm_rt_helpers */
#include "cm_mm_defines.h"

/*! \def TIMEOUT_MM_CM_MSG(x)
    \brief Timeout value (10s) for MM->CM messages
*/
#define TIMEOUT_MM_CM_MSG(x) (x * 1U)

/*! \typedef cm_state_e
    \brief CM current state
*/
typedef uint32_t cm_state_e;

/*! \enum cm_state
    \brief Possible CM states.
*/
enum cm_state {
    CM_STATE_NORMAL,    /**< CM is operation in normal state. */
    CM_STATE_HANG,      /**< CM is Hung. */
    CM_STATE_EXCEPTION  /**< CM took Exception. */
};

/*! \fn int32_t CM_Iface_Init(void)
    \brief Function to initialize messaging infrastucture to compute
    minions.
    \return Status success or error
*/
int32_t CM_Iface_Init(void);

/*! \fn int32_t CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message, uint64_t *failed_cm_shire_mask)
    \brief Function to multicast a message to the compute shires
    specified
    by the shire mask
    \param dest_shire_mask Destination shire mask
    \param message Pointer to message buffer
    \return Status success or error
*/
int32_t CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message, uint64_t *failed_cm_shire_mask);

/*! \fn int32_t CM_Iface_Unicast_Receive(uint64_t cb_idx,
    cm_iface_message_t *const message)
    \brief Function to receive any message from CM to MM unicast
    buffer.
    \param cb_idx Index of the unicast buffer
    \param message Pointer to message buffer
    \return Status success or error
    \warning Not thread safe. Only one caller per cb_idx.
    User needs to use the respective locking APIs if thread safety is required.
*/
int32_t CM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message);

int32_t CM_Iface_Update_CM_State(cm_state_e expected, cm_state_e desired);

#endif
