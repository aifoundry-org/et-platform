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
    \brief A C module that implements the compute worker related APIs.

    Public interfaces:
        CW_Init
        CW_Deinit
*/
/***********************************************************************/
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


/*! \struct cw_cb_t;
    \brief Compute Worker control block.
    Consists data structures to manage shire state of shires that play
    the role of compute workers 
*/
typedef struct cw_cb_t_{
    uint64_t shire_mask;
    uint64_t booted_shires_mask;
    cw_shire_state_t shire_status[NUM_SHIRES];
} cw_cb_t;

/*! \var cw_cb_t CW_CB
    \brief Global Compute Worker Control Block
    \warning Not thread safe!
*/
static cw_cb_t CW_CB __attribute__((aligned(64))) = {0};


int8_t CW_Update_Shire_State(uint64_t shire, cw_shire_state_t shire_state)
{
    (void)shire;
    (void)shire_state;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Init
*  
*   DESCRIPTION
*
*       Initialize Minions thats serve as compute workers
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
    uint64_t temp;
    int8_t status = STATUS_SUCCESS;

    minion_fw_boot_config_t *minion_fw_boot_config;

    /* Obtain the number of shires to be used from the boot
    configuration and initialize the CW control block */
    minion_fw_boot_config = 
        (minion_fw_boot_config_t*)FW_MINION_FW_BOOT_CONFIG;
    CW_CB.shire_mask =  
        minion_fw_boot_config->minion_shires & ((1ULL << NUM_SHIRES)-1);

    /* TODO: Shall be updated to use nee shires API */
    set_functional_shires(CW_CB.shire_mask);
    Log_Write(LOG_LEVEL_DEBUG, "Dispatcher:Set functional shires\r\n");

    /* Enable supervisor external and software interrupts, then enable
    interrupts */
    /* TODO: create and use proper macros from interrupts.h */
    asm volatile("li    %0, 0x202    \n"
                 "csrs  sie, %0      \n" 
                 "csrsi sstatus, 0x2 \n"
                 : "=&r"(temp));

    /* Bring up Compute Workers */
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, CW_CB.shire_mask, 0x1u, 0);

    /* Wait for al workers to be initialize */
    while(1) 
    {
        /* Break loop if all compute minions are booted and ready */
        /* TODO: Shall be updated to use nee shires API */
        if(all_shires_booted(CW_CB.shire_mask))
            break;

        /* Disable supervisor interrupts */
        asm volatile("csrci sstatus, 0x2"); 

        if (!swi_flag) 
        {
            asm volatile("wfi");
        }

        /* Enable supervisor interrupts */
        asm volatile("csrsi sstatus, 0x2"); 

        if(swi_flag)
        {
            swi_flag = false;

            /* Ensure flag clears before messages are handled */
            asm volatile("fence");

            /* Processess messages from CM from CM > MM unicast circbuff */
            while(1) 
            {
                cm_iface_message_t message;

                /* Unicast to dispatcher is slot 0 of unicast circular-buffers */
                /* TODO: This should be brought into proper abstraction in cw_iface.h */
                status = CM_To_MM_Iface_Unicast_Receive(0, &message);

                if (status != STATUS_SUCCESS)
                    break;

                switch (message.header.id) 
                {
                    case CM_TO_MM_MESSAGE_ID_NONE:
                    {
                        Log_Write(LOG_LEVEL_DEBUG, 
                            "Dispatcher:from CW:Invalid MESSAGE_ID_NONE received\r\n");
                        break;
                    }

                    case CM_TO_MM_MESSAGE_ID_SHIRE_READY: 
                    {
                        const mm_to_cm_message_shire_ready_t *shire_ready =
                            (const mm_to_cm_message_shire_ready_t *)&message;
                        Log_Write(LOG_LEVEL_DEBUG, "%s%llx%s",
                            "Dispatcher:from CW:MESSAGE_ID_SHIRE_READY received from shire 0x", 
                            shire_ready->shire_id, "\r\n");
                        /* TODO: Shall be updated to use nee shires API */
                        update_shire_state(shire_ready->shire_id, 
                            SHIRE_STATE_READY);
                        break;
                    }

                    default:
                    {
                        Log_Write(LOG_LEVEL_CRITICAL, "%s%llx%s",
                            "Dispatcher:from CW:Unknown message id = 0x", message.header.id, 
                            " received (unicast dispatcher)\r\n");
                        break;
                    }
                }
            };
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, 
                "Dispatcher:Unexpected condition, broke wfi without a SWI during CW boot...\r\n");
        }
    };

    return status;
}