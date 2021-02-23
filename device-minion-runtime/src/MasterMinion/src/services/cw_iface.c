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
/*! \file console.c
    \brief A C module that implements the Compute Worker Interface
    services

    Public interfaces:
        Worker_Iface_Init
*/
/***********************************************************************/
#include "services/cw_iface.h"
#include "services/log.h"

/* TODO: Move mm_to_cm_iface.c contents here */

int8_t CW_Iface_Processing(cm_iface_message_t *msg)
{
    /* Process as many requests as available */
    while (1) {
        cm_iface_message_t message;
        int8_t status;

        // Unicast to dispatcher is slot 0 of unicast circular-buffers
        status = CM_To_MM_Iface_Unicast_Receive(0, &message);
        if (status != STATUS_SUCCESS)
            break;

        switch (message.header.id) {
        case CM_TO_MM_MESSAGE_ID_NONE:
            Log_Write(LOG_LEVEL_DEBUG, "Invalid MESSAGE_ID_NONE received\r\n");
            break;

        case CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY: {
            const mm_to_cm_message_shire_ready_t *shire_ready =
                (const mm_to_cm_message_shire_ready_t *)&message;
            Log_Write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_SHIRE_READY received from shire %" PRId64 "\r\n",
                      shire_ready->shire_id);
            update_shire_state(shire_ready->shire_id, SHIRE_STATE_READY);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ACK: {
            const cm_to_mm_message_kernel_launch_ack_t *msg =
                (const cm_to_mm_message_kernel_launch_ack_t *)&message;
            Log_Write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_LAUNCH_ACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_kernel_state(msg->slot_index, KERNEL_STATE_RUNNING);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_NACK: {
            const cm_to_mm_message_kernel_launch_nack_t *msg =
                (const cm_to_mm_message_kernel_launch_nack_t *)&message;
            Log_Write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_LAUNCH_NACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_shire_state(msg->shire_id, SHIRE_STATE_ERROR);
            update_kernel_state(msg->slot_index, KERNEL_STATE_ERROR);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_ABORT_NACK: {
            const cm_to_mm_message_kernel_launch_nack_t *msg =
                (const cm_to_mm_message_kernel_launch_nack_t *)&message;
            Log_Write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_ABORT_NACK received from shire %" PRId64 "\r\n",
                      msg->shire_id);
            update_shire_state(msg->shire_id, SHIRE_STATE_ERROR);
            update_kernel_state(msg->slot_index, KERNEL_STATE_ERROR);
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
            Log_Write(LOG_LEVEL_DEBUG,
                      "MESSAGE_ID_KERNEL_COMPLETE received\r\n");
            update_kernel_state(((cm_to_mm_message_kernel_launch_completed_t *)&message)->slot_index,
                                KERNEL_STATE_COMPLETE);
            break;

        case CM_TO_MM_MESSAGE_ID_FW_EXCEPTION: {
            cm_to_mm_message_exception_t *exception = (cm_to_mm_message_exception_t *)&message;
            print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
                            exception->hart_id);
            // non-kernel exceptions are unrecoverable. Put the shire in error state
            update_shire_state((exception->hart_id) / 64u, SHIRE_STATE_ERROR);
            const int kernel = 0; // TODO: Properly get kernel_id....
            update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
            break;
        }

        case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION: {
            //cm_to_mm_message_exception_t *exception = (cm_to_mm_message_exception_t *)&message;
            //print_exception(exception->mcause, exception->mepc, exception->mtval, exception->mstatus,
            //                exception->hart_id);
            const int kernel = 0; // TODO: Properly get kernel_id....
            update_kernel_state(kernel, KERNEL_STATE_ERROR); // the kernel has failed
            break;
        }

        default:
            Log_Write(LOG_LEVEL_CRITICAL,
                      "Unknown message id = 0x%016" PRIx64 " received (unicast dispatcher)\r\n",
                      message.header.id);
            break;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Iface_Deinit
*
*   DESCRIPTION
*
*       Deinitialize compute worker interface
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t              status, success or failure
*
***********************************************************************/
int8_t CW_Iface_Deinit(void)
{
    return 0;
}
