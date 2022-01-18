/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
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
*       This file consists the main method that serves as the entry
*       point for the the c runtime.
*
*   FUNCTIONS
*
*       main
*
***********************************************************************/
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <system/layout.h>
#include <transports/mm_cm_iface/message_types.h>

#include "cm_mm_defines.h"
#include "cm_to_mm_iface.h"
#include "device_minion_runtime_build_configuration.h"
#include "log.h"
#include "mm_to_cm_iface.h"
#include "trace.h"

#include <stdint.h>

/* Global variable to keep track of Compute Minions boot */
static spinlock_t CM_Thread_Boot_Counter[NUM_SHIRES] = { 0 };

extern void trap_handler(void);

void __attribute__((noreturn)) main(void)
{
    int8_t status;
    const uint32_t shire_id = get_shire_id();
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;

    // Setup supervisor trap vector
    asm volatile("csrw  stvec, %0\n" : : "r"(&trap_handler));

    // Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    // But enable Supervisor Software Interrupts so that IPIs trap when in U-mode
    // RISC-V spec:
    //   "An interrupt i will be taken if bit i is set in both mip and mie,
    //    and if interrupts are globally enabled."
    asm volatile("csrci sstatus, 0x2\n");
    asm volatile("csrsi sie, %0\n" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    // Enable all available PMU counters to be sampled in U-mode
    asm volatile("csrw scounteren, %0\n" : : "r"(((1u << PMU_NR_HPM) - 1) << PMU_FIRST_HPM));

    /* Initialize Trace with default configurations. */
    Trace_Init_CM(NULL);

    /* Last thread in a shire sends shire ready message to Master Minion */
    if (atomic_add_local_32(&CM_Thread_Boot_Counter[shire_id].flag, 1U) == (thread_count - 1))
    {
        /* Reset the thread boot counter */
        init_local_spinlock(&CM_Thread_Boot_Counter[shire_id], 0);

        const mm_to_cm_message_shire_ready_t message = { .header.id =
                                                             CM_TO_MM_MESSAGE_ID_FW_SHIRE_READY,
            .shire_id = shire_id };

        /* To Master Shire thread 0 aka Dispatcher (circbuff queue index is 0) */
        status = CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (const cm_iface_message_t *)&message);

        if (status == 0)
        {
            /* Log boot message */
            log_write(LOG_LEVEL_CRITICAL, "Shire %d booted up!\r\n", shire_id);
        }
        else
        {
            log_write(LOG_LEVEL_ERROR, "CM->MM:Shire_ready:Unicast send failed! Error code: %d\n",
                status);
        }
    }

    /* Initialize the MM-CM Iface */
    MM_To_CM_Iface_Init();

    /* Start the main processing loop */
    MM_To_CM_Iface_Main_Loop();
}
