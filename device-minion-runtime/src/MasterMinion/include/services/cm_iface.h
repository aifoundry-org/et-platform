/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
    CM_STATE_NORMAL,   /**< CM is operation in normal state. */
    CM_STATE_HANG,     /**< CM is Hung. */
    CM_STATE_EXCEPTION /**< CM took Exception. */
};

/*! \fn int32_t CM_Iface_Init(bool reset_lock)
    \brief Function to initialize messaging infrastructure to compute
    minions.
    \param reset_lock Reset broadcast message lock.
    \return Status success or error
*/
int32_t CM_Iface_Init(bool reset_lock);

/*! \fn void CM_Iface_Multicast_Block(void)
    \brief Block MM to CM Multicast messages. It will wait for completion of
        in-progress messages (if any).
    \return None
*/
void CM_Iface_Multicast_Block(void);

/*! \fn void CM_Iface_Multicast_Unblock(void)
    \brief Unblock MM to CM Multicast messages.
    \return None
*/
void CM_Iface_Multicast_Unblock(void);

/*! \fn int32_t CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message)
    \brief Function to multicast a message to the compute shires
    specified
    by the shire mask
    \param dest_shire_mask Destination shire mask
    \param message Pointer to message buffer
    \return Status success or error
*/
int32_t CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message);

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

/*! \fn int32_t CM_Iface_Update_CM_State(cm_state_e expected, cm_state_e desired)
    \brief Function to receive any message from CM to MM unicast buffer.
    \param expected Expected current state of CM
    \param desired Desired new state of CM, it is updated only when
                   current state is expected
    \return Status success or error
*/
int32_t CM_Iface_Update_CM_State(cm_state_e expected, cm_state_e desired);

#endif
