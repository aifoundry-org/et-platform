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
/*! \file cm_to_mm_iface.h.h
    \brief A C header that defines the CM to MM public interfaces.
*/
/***********************************************************************/
#ifndef CM_TO_MM_IFACE_H
#define CM_TO_MM_IFACE_H

#include "cm_mm_defines.h"
#include "message_types.h"

/*! \fn int8_t CM_To_MM_Iface_Unicast_Receive(uint64_t cb_idx,
    cm_iface_message_t *const message)
    \brief Function to receive any message from CM to MM unicast buffer.
    Not thread safe. Only one caller per cb_idx
    \param cb_idx Index of the unicast buffer
    \param message Pointer to message buffer
    \return Status success or error
*/
int8_t CM_To_MM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message);

#endif
