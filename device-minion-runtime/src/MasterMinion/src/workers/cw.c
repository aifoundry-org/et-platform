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
/*! \file cw.c
    \brief A C module that implements the compute worker related
    public and private interfaces. This component provides interfaces
    to other components in the Master Minion runtime for initialization,
    and management, of compute shires available on the device. Additionally
    a helper fn is provided by this component to handle messages from
    compute minion S mode firmware.
    It implements the interfaces listed below;

    Public interfaces:
        CW_Init
        CW_Process_CM_SMode_Messages
        CW_Update_Shire_State
        CW_Check_Shires_Available_And_Ready
        CW_Get_Physically_Enabled_Shires
*/
/***********************************************************************/
#include "atomic.h"
#include "common_defs.h"
#include "minion_fw_boot_config.h"
#include "layout.h"
#include "workers/cw.h"
#include "services/cm_iface.h"
#include "services/log.h"
#include "syscall.h"
#include "syscall_internal.h"
#include "message_types.h"
#include "riscv_encoding.h"

/*! \typedef cw_cb_t
    \brief Compute Worker control block.
    Consists data structures to manage shire state of shires that play
    the role of compute workers
*/
typedef struct cw_cb_t_{
    uint64_t physically_avail_shires_mask;
    uint64_t booted_shires_mask;
    uint64_t shire_state;
} cw_cb_t;

/*! \var cw_cb_t CW_CB
    \brief Global Compute Worker Control Block
    \warning Not thread safe!
*/
static cw_cb_t CW_CB __attribute__((aligned(64))) = {0};

/************************************************************************
*
*   FUNCTION
*
*       CW_Init
*
*   DESCRIPTION
*
*       Initialize Shires/Minions that serve as compute workers
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
int8_t CW_Init(void)
{
    uint64_t sip;
    uint64_t shire_mask;
    uint64_t booted_shires_mask = 0ULL;
    int8_t status = STATUS_SUCCESS;

    minion_fw_boot_config_t *minion_fw_boot_config;

    /* Obtain the number of shires to be used from the boot
    configuration and initialize the CW control block */
    minion_fw_boot_config =
        (minion_fw_boot_config_t*)FW_MINION_FW_BOOT_CONFIG;
    shire_mask =
        minion_fw_boot_config->minion_shires & ((1ULL << NUM_SHIRES)-1);

    /* Initialize Global CW_CB */
    atomic_store_local_64(&CW_CB.physically_avail_shires_mask,
        shire_mask);
    atomic_store_local_64(&CW_CB.booted_shires_mask, 0ULL);

    atomic_store_local_64(&CW_CB.shire_state, 0ULL);

    /* Bring up Compute Workers */
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, shire_mask, 0x1u, 0);

    /* Wait for all workers to be initialized */
    while(1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        if(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Processess messages from CM from CM > MM unicast circbuff */
            while(1)
            {
                cm_iface_message_t message;
                int8_t internal_status;

                /* Unicast to dispatcher is slot 0 of unicast
                circular-buffers */
                internal_status = CM_Iface_Unicast_Receive
                    (CM_MM_MASTER_HART_UNICAST_BUFF_IDX, &message);

                if (internal_status != STATUS_SUCCESS)
                    break;

                switch (message.header.id)
                {
                    case CM_TO_MM_MESSAGE_ID_NONE:
                    {
                        Log_Write(LOG_LEVEL_DEBUG,
                            "Dispatcher:CW_Init:MESSAGE_ID_NONE\r\n");

                        break;
                    }

                    case CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY:
                    {
                        const mm_to_cm_message_shire_ready_t *shire_ready =
                            (const mm_to_cm_message_shire_ready_t *)&message;

                        Log_Write(LOG_LEVEL_DEBUG,
                            "Dispatcher:CW_Init:MESSAGE_ID_SHIRE_READY S%d\r\n",
                            shire_ready->shire_id);

                        /* Update the booted shire mask */
                        booted_shires_mask |= (1ULL << shire_ready->shire_id);
                        break;
                    }

                    default:
                    {
                        Log_Write(LOG_LEVEL_ERROR,
                            "Dispatcher:CW_Init:Unknown message id = 0x%x\r\n",
                            message.header.id);
                        break;
                    }
                }
            }

            /* Break loop if all compute minions are booted and ready */
            if((booted_shires_mask & shire_mask) == shire_mask)
            {
                atomic_store_local_64(&CW_CB.booted_shires_mask,
                                      booted_shires_mask);
                break;
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR,
                "Dispatcher:CW_Init:Unexpected condition, \
                broke wfi without a SWI during CW_Init...\r\n");
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Process_CM_SMode_Messages
*
*   DESCRIPTION
*
*       Helper to process messages from compute minion firmware running
*       in S mode. Specifically handles exceptions that arise in during
*       compute minion firmware in S mode.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
void CW_Process_CM_SMode_Messages(void)
{
    /* Processes messages from CM from CM > MM unicast circbuff */
    while(1)
    {
        cm_iface_message_t message;

        /* Get the message from unicast buffer */
        if (CM_Iface_Unicast_Receive
            (CM_MM_MASTER_HART_UNICAST_BUFF_IDX, &message) != STATUS_SUCCESS)
        {
            break;
        }

        switch (message.header.id)
        {
            case CM_TO_MM_MESSAGE_ID_NONE:
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "Dispatcher:CM_TO_MM:MESSAGE_ID_NONE\r\n");
                break;
            }

            case CM_TO_MM_MESSAGE_ID_FW_EXCEPTION:
            {
                cm_to_mm_message_exception_t *exception =
                    (cm_to_mm_message_exception_t *)&message;

                Log_Write(LOG_LEVEL_CRITICAL,
                    "Dispatcher:CM_TO_MM:\
                    MESSAGE_ID_FW_EXCEPTION from H%ld\r\n",
                    exception->hart_id);

                /* TODO: SW-6569: CW FW exception received.
                Decode exception and reset the FW */

                break;
            }
            default:
            {
                Log_Write(LOG_LEVEL_CRITICAL,
                    "Dispatcher:CM_TO_MM:Unknown message id = 0x%x\r\n",
                    message.header.id);
                break;
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Get_Physically_Enabled_Shires
*
*   DESCRIPTION
*
*       Get the actual physically available shires
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    Get available shires
*
***********************************************************************/
uint64_t CW_Get_Physically_Enabled_Shires(void)
{
    return atomic_load_local_64(&CW_CB.physically_avail_shires_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Get_Booted_Shires
*
*   DESCRIPTION
*
*       Get the booted available shires
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    Returns booted shires mask
*
***********************************************************************/
uint64_t CW_Get_Booted_Shires(void)
{
    return atomic_load_local_64(&CW_CB.booted_shires_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Update_Shire_State
*
*   DESCRIPTION
*
*       Set the state of a group of shires
*
*   INPUTS
*
*       shire_mask    Shire group mask
*       shire_state   Shire state
*
*   OUTPUTS
*
*       int8_t        status success or failure
*
***********************************************************************/
void CW_Update_Shire_State(uint64_t shire_mask, cw_shire_state_t shire_state)
{
    if (shire_state == CW_SHIRE_STATE_FREE)
    {
        atomic_and_local_64(&CW_CB.shire_state, ~shire_mask);
    }
    else
    {
        atomic_or_local_64(&CW_CB.shire_state, shire_mask);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Check_Shires_Available_And_Free
*
*   DESCRIPTION
*
*       Check all shires specified by mask are available and ready.
*
*   INPUTS
*
*       uint64_t   Shire mask to check
*
*   OUTPUTS
*
*       int8t_t    Success status or error code
*
***********************************************************************/
int8_t CW_Check_Shires_Available_And_Free(uint64_t shire_mask)
{
    int8_t status = CW_SHIRES_NOT_FREE;

    /* Verify if all the given shires are booted */
    if((atomic_load_local_64(&CW_CB.booted_shires_mask) &
        shire_mask) == shire_mask)
    {
        /* Shire state definition is 0 if free else 1 if busy */
        /* Hence flips status to compare with Requested Shire Mask */
        if((~atomic_load_local_64(&CW_CB.shire_state) & shire_mask) == shire_mask)
        {
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        status = CW_SHIRE_UNAVAILABLE;
    }

    return status;
}
