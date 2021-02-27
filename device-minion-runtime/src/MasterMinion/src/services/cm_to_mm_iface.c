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
/*! \file cm_to_mm_iface.c
    \brief A C module that implements the CM to MM related public and
    private interfaces.

    Public interfaces:
        CM_To_MM_Iface_Unicast_Receive
        CM_To_MM_Iface_Processing
*/
/***********************************************************************/
#include "layout.h"
#include "circbuff.h"
#include "services/cm_to_mm_iface.h"

/************************************************************************
*
*   FUNCTION
*
*       CM_To_MM_Iface_Unicast_Receive
*
*   DESCRIPTION
*
*       Function to receive any message from CM to MM unicast buffer.
*
*   INPUTS
*
*       cb_idx    Index of the unicast buffer
*       message   Pointer to message buffer
*
*   OUTPUTS
*
*       int8_t    status success or failure
*
***********************************************************************/
int8_t CM_To_MM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message)
{
    int8_t status;
    circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                            cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);

    /* Peek the command size to pop */
    status = Circbuffer_Peek(cb, (void *)&message->header, 0,
        sizeof(message->header), L3_CACHE);

    if (status == STATUS_SUCCESS)
    {
        /* Pop the command from circular buffer */
        status = Circbuffer_Pop(cb, message, sizeof(*message), L3_CACHE);
    }

    return status;
}
