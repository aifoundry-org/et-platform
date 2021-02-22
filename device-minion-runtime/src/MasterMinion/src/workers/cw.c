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
    public and private interfaces. The Compute Worker provides interfaces
    to other components present in the Master Minion runtime that
    facilitate management of compute workers and shires associated
    with them. It implements the interfaces listed below;

    Public interfaces:
        CW_Init
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
#include "services/log1.h"
#include "syscall.h"
#include "syscall_internal.h"
#include "swi.h"
#include "shire.h"
#include "message_types.h"
#include "cm_to_mm_iface.h"
#include "riscv_encoding.h"

/*! \typedef cw_cb_t
    \brief Compute Worker control block.
    Consists data structures to manage shire state of shires that play
    the role of compute workers
*/
typedef struct cw_cb_t_{
    uint64_t physically_avail_shires_mask;
    uint64_t booted_shires_mask;
    cw_shire_state_t shire_status[NUM_SHIRES];
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
    int8_t status = STATUS_SUCCESS;

    minion_fw_boot_config_t *minion_fw_boot_config;

    /* Obtain the number of shires to be used from the boot
    configuration and initialize the CW control block */
    minion_fw_boot_config =
        (minion_fw_boot_config_t*)FW_MINION_FW_BOOT_CONFIG;
    shire_mask =
        minion_fw_boot_config->minion_shires & ((1ULL << NUM_SHIRES)-1);

    /* Initialize Global CW_CB */
    atomic_store_local_64(&CW_CB.physically_avail_shires_mask, shire_mask);
    atomic_store_local_64(&CW_CB.booted_shires_mask, 0U);

    for(uint8_t shire = 0; shire < NUM_SHIRES; shire++)
    {
        atomic_store_local_8(&CW_CB.shire_status[shire],
            CW_SHIRE_STATE_UNKNOWN);
    }

    /* Bring up Compute Workers */
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, shire_mask, 0x1u, 0);

    /* Wait for all workers to be initialized */
    while(1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        asm volatile("csrr %0, sip" : "=r"(sip));

        if(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Processess messages from CM from CM > MM unicast circbuff */
            while(1)
            {
                cm_iface_message_t message;

                /* Unicast to dispatcher is slot 0 of unicast
                circular-buffers */
                /* TODO: This should be brought into proper abstraction
                in cw_iface.h */
                status = CM_To_MM_Iface_Unicast_Receive
                    (CM_MM_MASTER_HART_UNICAST_BUFF_IDX, &message);

                if (status != STATUS_SUCCESS)
                    break;

                switch (message.header.id)
                {
                    case CM_TO_MM_MESSAGE_ID_NONE:
                    {
                        Log_Write(LOG_LEVEL_DEBUG, "Dispatcher:from CW:MESSAGE_ID_NONE\r\n");
                        break;
                    }

                    case CM_TO_MM_MESSAGE_ID_SHIRE_READY:
                    {
                        const mm_to_cm_message_shire_ready_t *shire_ready =
                            (const mm_to_cm_message_shire_ready_t *)&message;

                        Log_Write(LOG_LEVEL_DEBUG,
                            "Dispatcher:from CW:MESSAGE_ID_SHIRE_READY S%d\r\n",
                            shire_ready->shire_id);

                        /* Update the shire state in CW CB */
                        CW_Update_Shire_State(shire_ready->shire_id,
                            CW_SHIRE_STATE_READY);
                        break;
                    }

                    default:
                    {
                        Log_Write(LOG_LEVEL_CRITICAL,
                            "Dispatcher:from CW:Unknown message id = 0x%x\r\n",
                            message.header.id);
                        break;
                    }
                }
            }

            /* Break loop if all compute minions are booted and ready */
            if((atomic_load_local_64
                (&CW_CB.booted_shires_mask) & shire_mask) == shire_mask)
                break;
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "Dispatcher:Unexpected condition, broke wfi without a SWI during CW boot...\r\n");
        }
    };

    return status;
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
*       CW_Update_Shire_State
*
*   DESCRIPTION
*
*       Set the state of a shire
*
*   INPUTS
*
*       shire         Shire number
*       shire_state   Shire state
*
*   OUTPUTS
*
*       int8_t        status success or failure
*
***********************************************************************/
int8_t CW_Update_Shire_State(uint64_t shire, cw_shire_state_t shire_state)
{
    int8_t status = STATUS_SUCCESS;

    if (atomic_load_local_8(&CW_CB.shire_status[shire]) !=
        CW_SHIRE_STATE_ERROR)
    {
        atomic_store_local_8(&CW_CB.shire_status[shire], shire_state);

        /* Update mask of booted shires */
        atomic_or_local_64(&CW_CB.booted_shires_mask, 1ULL << shire);
    }
    else
    {
        /* The only legal transition from ERROR state is to READY state */
        if (shire_state == CW_SHIRE_STATE_READY)
        {
            atomic_store_local_8(&CW_CB.shire_status[shire], shire_state);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "CW:Error:Illegal transition from error:Shire:%" PRId64 "\r\n", shire);

            status = GENERAL_ERROR;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Check_Shires_Available_And_Ready
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
int8_t CW_Check_Shires_Available_And_Ready(uint64_t shire_mask)
{
    int8_t status = STATUS_SUCCESS;

    /* Verify if all the given shires are physically available */
    if ((atomic_load_local_64(&CW_CB.physically_avail_shires_mask) &
        shire_mask) == shire_mask)
    {
        /* Check the provided the shire_mask for ready state */
        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
        {
            if (shire_mask & (1ULL << shire))
            {
                if (atomic_load_local_8(&CW_CB.shire_status[shire]) !=
                    CW_SHIRE_STATE_READY)
                {
                    status = CW_SHIRES_NOT_READY;
                    break;
                }
            }
        }
    }
    else
    {
        status = CW_SHIRE_UNAVAILABLE;
    }

    return status;
}
