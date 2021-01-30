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
/*! \file dispatcher.c
    \brief A C module that implements the Dispatcher. The function of
    the Dispatcher is to;
    1. Initialize system components
        Serial
        Trace
        Interrupts
    2. Initialize interfaces;
        Host Interface
        SP Interface
        CW Interface
    2. Initialize Workers;
        Submission Queue Workers
        Kernel Worker
        DMA Worker
        Compute Workers
    3. Initialize Device Interface Registers
    4. Infinite loop - that fields interrupts and dispatches appropriate
    processing;
        Field Host PCIe interrupts and dispatch command processing
        Field SP IPIs and dispatch command processing
*/
/***********************************************************************/
#include "minion_fw_boot_config.h"
#include "config/dir_regs.h"
#include "dispatcher/dispatcher.h"
#include "workers/sqw.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "workers/cw.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/sp_iface.h"
#include "services/log1.h"
#include "services/lock.h"
#include "drivers/interrupts.h"
#include "syscall_internal.h"
#include "serial.h"
#include "message_types.h"
#include "sync.h"
#include "fcc.h"
#include "pmu.h"
#include "swi.h" /* TODO: Need to update swi.c to new coding conventions and abstraction */

extern spinlock_t Launch_Wait;

extern bool Host_Iface_Interrupt_Flag;

/* TODO: This shoul dbe included using a proper header during clean up */
extern void message_init_master(void);

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
    /* Initialize Serial Interface */
    SERIAL_init(UART0);

    Log_Write(LOG_LEVEL_CRITICAL, "%s = %d %s",
        "Dispatcher: launched on HART:" , hart_id, "\r\n");

    /* Initialize Device Interface Registers */
    DIR_Init();
    Log_Write(LOG_LEVEL_DEBUG, "%s",
        "Dispatcher: Device Interface Registers initialized\r\n");

    /* Enable interrupt resources */
    Interrupt_Init();
    Log_Write(LOG_LEVEL_DEBUG, "%s",
        "Dispatcher: Interrupts initialized\r\n");
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_INTERRUPT_INITIALIZED);

    /* Reset PMC cycles counter */
    PMC_RESET_CYCLES_COUNTER;

    /* TODO: Needs to be updated to proper coding standard and
    abstractions */
    message_init_master();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_CM_INTERFACE_READY);

    /* Init FCCs for current minion */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    /* Initialize Computer Workers */
    CW_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_CM_WORKERS_INITIALIZED);

    /* Initialize Master Shire Workers */
    SQW_Init();
    KW_Init();
    DMAW_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_WORKERS_INITIALIZED);

    /* Initialize Host Submission Queue and Completion Queue Interface */
    Host_Iface_SQs_Init();
    Host_Iface_CQs_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_HOST_VQ_READY);

    /* Initialize Service Processor Submission Queue and Completion
    Queue Interface */
    SP_Iface_SQs_Init();
    SP_Iface_CQs_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_INTERFACE_READY);

    /* Release Master Shire Workers */
    Log_Write(LOG_LEVEL_DEBUG, "%s",
        "Dispatcher: Releasing workers\r\n");
    local_spinwait_set(&Launch_Wait, 1);

    /* Mark Master Minion Status as Ready */
    /* Now able to receive and process commands from host .. */
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_READY);

    Log_Write(LOG_LEVEL_DEBUG, "%s",
        "Dispatcher: Master Minion READY!\r\n");

    /* Wait for a message from the host, SP, worker minions etc. */
    while(1)
    {
        INTERRUPTS_DISABLE_SUPERVISOR;

        if(!Host_Iface_Interrupt_Status())
        {
            WAIT_FOR_INTERRUPTS;
            Log_Write(LOG_LEVEL_DEBUG, "%s",
                "Dispatcher: Exiting WFI!\r\n");
        }

        INTERRUPTS_ENABLE_SUPERVISOR;

        Host_Iface_Processing();
        SP_Iface_Processing();
    }

    return;
}
