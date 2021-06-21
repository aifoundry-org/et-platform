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
    \brief A C module that implements the Dispatcher thread.
    The function of the Dispatcher is to
    1. Initialize system components
        Serial
        Trace
        Interrupts
    2. Initialize interfaces
        Host Interface
        SP Interface
        CM Interface
    2. Initialize Workers
        Submission Queue Workers
        Kernel Worker
        DMA Worker
        Compute Workers
    3. Initialize Device Interface Registers
    4. Spin in infinite loop - that fields interrupts and dispatches
    appropriate processing
        Field Host PCIe interrupts and dispatch command processing
        Field SP IPIs and dispatch command processing
        Field CM IPIs and dispatch handling of messages from CMs
*/
/***********************************************************************/
#include "config/dir_regs.h"
#include "dispatcher/dispatcher.h"
#include "workers/sqw.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "workers/cw.h"
#include "services/cm_iface.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/sp_iface.h"
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "drivers/plic.h"
#include "serial.h"
#include "message_types.h"
#include "sync.h"
#include "device-common/fcc.h"
#include "pmu.h"
#include "riscv_encoding.h"

extern spinlock_t Launch_Wait;

extern bool Host_Iface_Interrupt_Flag;

extern bool SW_Timer_Interrupt_Flag;

/* Local functions */

static inline void dispatcher_assert(bool condition, mm2sp_sp_recoverable_error_code_e error,
    const char* error_log)
{
    if (!condition)
    {
        /* Report error in DIRs */
        DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_FW_ERROR);

        SP_Iface_Report_Error(SP_RECOVERABLE, error);

        /* Assert with failure */
        ASSERT(false, error_log);
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

    /* Initialize Trace for Master Minions in default configuration. */
    Trace_Init_MM(NULL);

   /* Initialize Service Processor interface, it consists
    1. MM to SP SQ, and MM to SQ CQ
    2. SP to MM SQ, and Sp to MM CQ */
    status = SP_Iface_Init();
    dispatcher_assert(status == STATUS_SUCCESS, MM_SP_IFACE_INIT_ERROR,
                                                "Service Processor init failure.");

    /* Initialize Serial Interface */
    status = (int8_t)SERIAL_init(UART0);
    dispatcher_assert(status == STATUS_SUCCESS, MM_SERIAL_INIT_ERROR,
                                                        "Serial init failure.");

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

    /* Reset PMC cycles counter for all Harts
    (can be even or odd depending upong hart ID) in the Neighbourhood */
    PMC_RESET_CYCLES_COUNTER;

    /* Initialize the interface to compute minion */
    status = CM_Iface_Init();
    dispatcher_assert(status == STATUS_SUCCESS, MM_CM_IFACE_INIT_ERROR,
                                        "Compute Workers's interface init failure.");
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_CM_INTERFACE_READY);

    /* Also enable waking from WFI on supervisor external and timer interrupts (IPIs),
    Host interrupts go to the PLIC, which are received as external interrupts. */
    asm volatile("csrs  sie, %0\n"
            : : "r" ((1 << SUPERVISOR_EXTERNAL_INTERRUPT) |
            (1 << SUPERVISOR_TIMER_INTERRUPT)));

    /* Initialize SW Timer to register timeouts for commands */
    status = SW_Timer_Init();
    dispatcher_assert(status == SW_TIMER_OPERATION_SUCCESS, MM_CW_INIT_ERROR,
                                                        "SW Timer init failure.");

    /* Setup MM->SP Heartbeat */
    status = SP_Iface_Setup_MM_HeartBeat();
    dispatcher_assert(status == STATUS_SUCCESS, MM_CW_INIT_ERROR,
                                                    "MM->SP Heartbeat init failure.");

    /* Initialize Computer Workers */
    status = CW_Init();
    dispatcher_assert(status == STATUS_SUCCESS, MM_CW_INIT_ERROR,
                                                    "Compute Workers init failure.");

    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_CM_WORKERS_INITIALIZED);

    /* Initialize Master Shire Workers */
    SQW_Init();
    KW_Init();
    DMAW_Init();
    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_WORKERS_INITIALIZED);

    /* Initialize Host Submission Queue and Completion Queue Interface */
    status = Host_Iface_SQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, MM_SQ_INIT_ERROR,
                                                        "Host SQs init failure.");

    status = Host_Iface_CQs_Init();
    dispatcher_assert(status == STATUS_SUCCESS, MM_CQ_INIT_ERROR,
                                                        "Host CQs init failure.");

    DIR_Set_Master_Minion_Status(MM_DEV_INTF_MM_BOOT_STATUS_MM_HOST_VQ_READY);
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

    /* Wait for a message from the host, SP, worker minions etc. */
    while(1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        Log_Write(LOG_LEVEL_DEBUG,
            "Dispatcher:Exiting WFI! SIP: 0x%" PRIx64 "\r\n", sip);

        if(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrc sip, %0" : : "r"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Process the messages from Compute Minions */
            CW_Process_CM_SMode_Messages();
        }

        if(sip & (1 << SUPERVISOR_EXTERNAL_INTERRUPT))
        {
            PLIC_Dispatch();

            if(Host_Iface_Interrupt_Status())
            {
                Host_Iface_Processing();
            }

            if(SW_Timer_Interrupt_Status())
            {
                SW_Timer_Processing();
            }
        }

        if(sip & (1 << SUPERVISOR_TIMER_INTERRUPT))
        {
            /* TODO: set new mtimecp, timer_tick()*/
        }
    } /* loop never breaks */

    /* no return */
}
