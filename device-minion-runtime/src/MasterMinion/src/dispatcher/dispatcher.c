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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements the Dispatcher. The function of the 
*       dispatcher is to field all interrupts and 
*
*   FUNCTIONS
*
*       Dispatcher_Launch
*
***********************************************************************/
#include "minion_fw_boot_config.h"
#include "config/dir_regs.h"
#include "dispatcher/dispatcher.h"
#include "workers/sqw.h"
#include "workers/cqw.h"
#include "workers/kw.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/sp_iface.h"
#include "services/worker_iface.h"
#include "services/log1.h"
#include "services/lock.h"
#include "drivers/interrupts.h"
#include "drivers/shires.h"
#include "syscall_internal.h"
#include "serial.h"
#include "message.h"
#include "sync.h"
#include "fcc.h"
#include "swi.h" /* TODO: Need to update swi.c to new coding conventions and abstraction */

extern spinlock_t Launch_Lock;

extern bool Host_Iface_Interrupt_Flag;

/************************************************************************
*
*   FUNCTION
*
*       Dispatcher_Launch
*  
*   DESCRIPTION
*
*       Launch a dispatcher instance on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Dispatcher_Launch(uint32_t hart_id)
{
    (void) hart_id;
    uint64_t temp;
    volatile minion_fw_boot_config_t *boot_config =
        (volatile minion_fw_boot_config_t *)FW_MINION_FW_BOOT_CONFIG;
    uint64_t functional_shires = 
        boot_config->minion_shires & ((1ULL << NUM_SHIRES) - 1);
    #if 0
    bool process_host_iface, process_sp_iface, process_worker_iface;
    process_host_iface = process_sp_iface = process_worker_iface = false;
    #endif

    /* Initialize Serial Interface */
    SERIAL_init(UART0);
    Log_Set_Level(LOG_LEVEL_DEBUG);
    Log_Write(LOG_LEVEL_DEBUG, "%s = %d %s", 
        "Dispatcher: launched on HART:" , hart_id, "\r\n");

    /* Enable interrupt resources */
    Interrupt_Init();
    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: Interrupts initialized \r\n");

    /* Initialize Workers */
    SQW_Init();
    CQW_Init();
    /* Initialize KW */
    /* Initialize DMAW */
    
    /* Host, and SP Interface Initializeation */
    Host_Iface_SQs_Init();
    Host_Iface_CQs_Init();
    #if 0
    SP_Iface_SQs_Init();
    SP_Iface_CQs_Init();
    #endif

    /* Initialize Device Interface Registers */
    DIR_Init();
    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: Device Interface Registers initialized \r\n");

    /* Initialize FIFO buffers to workers */
    Worker_Iface_Init(TO_KW_FIFO);
    Worker_Iface_Init(TO_DMAW_FIFO);
    Worker_Iface_Init(TO_CQW_FIFO);
    
    /* Init FCCs for current minion */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    /* Set currently active/functional shires */
    Shire_Set_Active(functional_shires);

    /* TODO: This should be a service provided by interrupts.h */
    /* Enable supervisor external and software interrupts */
    asm volatile("li    %0, 0x202    \n"
                 "csrs  sie, %0      \n"
                 "csrsi sstatus, 0x2 \n"
                 : "=&r"(temp));

#if 0 /* Please do not review code inside this #if, this is WIP */
    /* Bring up Compute Minions */
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, functional_shires, 
        0x1u, 0);
    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: Compute Minions configured \r\n");


    /* Block here till all shires are booted */
    while(1) 
    {
        if(Shire_Check_All_Are_Booted(functional_shires))
            break;
        
        INTERRUPTS_DISABLE_SUPERVISOR;
        
        if (!swi_flag) 
        {
            WAIT_FOR_INTERRUPTS;
        }

        INTERRUPTS_ENABLE_SUPERVISOR;

        /* TODO: This should be a service provided by SWI component */
        if (swi_flag) 
        {
            swi_flag = false;

            // Ensure flag clears before messages are handled
            asm volatile("fence");

            /* Check for and handle Compute Minion messages */
            CW_Message_Processing();

            /* Check for and handle SP messages */
        }

        /* Check for and hendle Timer events */
    }

#endif    
    
    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: Releasing workers \r\n");

    release_local_spinlock(&Launch_Lock);

    /* Update status to indicate MM is ready to use */
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_READY);

    Log_Write(LOG_LEVEL_DEBUG, "%s", 
        "Dispatcher: Master Minion READY! \r\n");

    /* Wait for a message from the host, SP, worker minions etc. */
    while(1)
    {
        INTERRUPTS_DISABLE_SUPERVISOR;

        if(!Host_Iface_Interrupt_Status())
        {
            WAIT_FOR_INTERRUPTS;
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "Dispatcher: Exiting WFI! \r\n");
        }

        INTERRUPTS_ENABLE_SUPERVISOR;

        Host_Iface_Processing();
    }

    return;
}
