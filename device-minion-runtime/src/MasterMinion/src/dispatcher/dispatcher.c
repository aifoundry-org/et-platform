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
#include "services/cm_to_mm_iface.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/sp_iface.h"
#include "services/log.h"
#include "drivers/plic.h"
#include "syscall_internal.h"
#include "serial.h"
#include "message_types.h"
#include "sync.h"
#include "fcc.h"
#include "pmu.h"
#include "riscv_encoding.h"

extern spinlock_t Launch_Wait;

extern bool Host_Iface_Interrupt_Flag;

/* TODO: This shoul dbe included using a proper header during clean up */
extern void message_init_master(void);

/* Local functions */

static inline void dispatcher_assert(bool condition, const char* error_log)
{
    if (!condition)
    {
        /* Report error in DIRs */
        DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_ERROR);

        /* TODO: Report error to SP */

        /* Assert with failure */
        ASSERT(false, error_log);
    }
}

static inline void dispatcher_process_cm_messages(void)
{
    /* Processes messages from CM from CM > MM unicast circbuff */
    while(1)
    {
        cm_iface_message_t message;

        /* Get the message from unicast buffer */
        if (CM_To_MM_Iface_Unicast_Receive(CM_MM_MASTER_HART_UNICAST_BUFF_IDX, &message) !=
            STATUS_SUCCESS)
        {
            break;
        }

        switch (message.header.id)
        {
            case CM_TO_MM_MESSAGE_ID_NONE:
            {
                Log_Write(LOG_LEVEL_DEBUG, "Dispatcher:CM_TO_MM:MESSAGE_ID_NONE\r\n");
                break;
            }

            case CM_TO_MM_MESSAGE_ID_FW_EXCEPTION:
            {
                cm_to_mm_message_exception_t *exception =
                    (cm_to_mm_message_exception_t *)&message;

                Log_Write(LOG_LEVEL_CRITICAL,
                    "Dispatcher:CM_TO_MM:MESSAGE_ID_FW_EXCEPTION from H%ld\r\n",
                    exception->hart_id);

                /* TODO: SW-6569: CW FW exception received. Decode exception and reset the FW */

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
    int8_t status;
    uint64_t sip;

    /* Initially set DIRs status to not ready */
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_NOT_READY);

    /* Initialize Serial Interface */
    status = (int8_t)SERIAL_init(UART0);
    dispatcher_assert(status == STATUS_SUCCESS, "Serial init failure.");

    Log_Write(LOG_LEVEL_CRITICAL,
        "Dispatcher:launched on H%d\r\n", hart_id);

    /* Initialize Device Interface Registers */
    DIR_Init();
    Log_Write(LOG_LEVEL_DEBUG,
        "Dispatcher:Device Interface Registers initialized\r\n");

    /* Initialize PLIC driver */
    PLIC_Init();
    Log_Write(LOG_LEVEL_DEBUG,
        "Dispatcher:PLIC initialized\r\n");
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_INTERRUPT_INITIALIZED);

    /* Reset PMC cycles counter */
    PMC_RESET_CYCLES_COUNTER;

    /* TODO: Needs to be updated to proper coding standard and
    abstractions */
    message_init_master();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_CM_INTERFACE_READY);

    /* Initialize Computer Workers */
    status = CW_Init();
    dispatcher_assert(status == STATUS_SUCCESS, "Compute Workers init failure.");

    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_CM_WORKERS_INITIALIZED);

    /* Initialize Master Shire Workers */
    SQW_Init();
    KW_Init();
    DMAW_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_WORKERS_INITIALIZED);

    /* Initialize Host Submission Queue and Completion Queue Interface */
    status = Host_Iface_SQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, "Host SQs init failure.");

    status = Host_Iface_CQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, "Host CQs init failure.");

    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_HOST_VQ_READY);

    /* Initialize Service Processor Submission Queue and Completion
    Queue Interface */
    status = SP_Iface_SQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, "SP SQs init failure.");

    status = SP_Iface_CQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, "SP CQs init failure.");

    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_INTERFACE_READY);

    /* Release Master Shire Workers */
    Log_Write(LOG_LEVEL_DEBUG,
        "Dispatcher:Releasing workers\r\n");
    local_spinwait_set(&Launch_Wait, 1);

    /* Mark Master Minion Status as Ready */
    /* Now able to receive and process commands from host .. */
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_READY);

    Log_Write(LOG_LEVEL_DEBUG,
        "Dispatcher:Master Minion READY!\r\n");

    /* Also enable waking from WFI on supervisor external and timer interrupts (IPIs),
    Host interrupts go to the PLIC, which are received as external interrupts. */
    asm volatile("csrs  sie, %0\n"
                 : : "r" ((1 << SUPERVISOR_EXTERNAL_INTERRUPT) |
                          (1 << SUPERVISOR_TIMER_INTERRUPT)));

    /* Wait for a message from the host, SP, worker minions etc. */
    while(1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        asm volatile("csrr %0, sip" : "=r"(sip));

        Log_Write(LOG_LEVEL_DEBUG,
            "Dispatcher:Exiting WFI! SIP: 0x%" PRIx64 "\r\n", sip);

        if(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrc sip, %0" : : "r"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            SP_Iface_Processing();

            /* Process the messages from Compute Minions */
            dispatcher_process_cm_messages();
        }

        if(sip & (1 << SUPERVISOR_EXTERNAL_INTERRUPT))
        {
            PLIC_Dispatch();

            if(Host_Iface_Interrupt_Status())
            {
                Host_Iface_Processing();
            }
        }

        if(sip & (1 << SUPERVISOR_TIMER_INTERRUPT))
        {
            /* TODO: set new mtimecp, timer_tick()*/
        }
    }

    return;
}
