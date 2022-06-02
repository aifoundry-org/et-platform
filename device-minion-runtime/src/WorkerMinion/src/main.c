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
#include "etsoc/common/common_defs.h"
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

/*! \def CM_BOOT_MASK_PTR
    \brief Global shared pointer to CM shires boot mask
*/
#define CM_BOOT_MASK_PTR ((uint64_t *)CM_SHIRES_BOOT_MASK_BASEADDR)

extern void trap_handler(void);

void __attribute__((noreturn)) main(void)
{
    const uint32_t shire_id = get_shire_id();
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;
    int32_t status = GENERAL_ERROR;

    /* Setup supervisor trap vector */
    asm volatile("csrw  stvec, %0\n" : : "r"(&trap_handler));

    /* Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    But enable Supervisor Software Interrupts so that IPIs and bus error interrupts
    trap when in U-mode
    RISC-V spec:
       "An interrupt i will be taken if bit i is set in both mip and mie,
        and if interrupts are globally enabled." */
    SUPERVISOR_INTERRUPTS_DISABLE
    asm volatile("csrs sie, %0\n"
                 :
                 : "r"((1 << SUPERVISOR_SOFTWARE_INTERRUPT) | (1 << BUS_ERROR_INTERRUPT)));

    /* Enable all available PMU counters to be sampled in U-mode */
    asm volatile("csrw scounteren, %0\n" : : "r"(((1u << PMU_NR_HPM) - 1) << PMU_FIRST_HPM));

    /* Initialize the MM-CM Iface */
    MM_To_CM_Iface_Init();

    /* Initialize Trace with default configurations. */
    status = Trace_Init_CM(NULL);
    if (status != STATUS_SUCCESS)
    {
        cm_to_mm_message_fw_error_t message = { .header.id = CM_TO_MM_MESSAGE_ID_FW_ERROR,
            .hart_id = get_hart_id(),
            .error_code = status };

        /* Send error message to dispatcher (Master shire Hart 0) */
        CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (cm_iface_message_t *)&message);
    }

    /* Last thread in a shire updates the global CM shire boot mask */
    uint32_t booted_threads = atomic_add_local_32(&CM_Thread_Boot_Counter[shire_id].flag, 1U) + 1;
    if (booted_threads == thread_count)
    {
        /* TODO:SW-11626: This is workaround for Zebu CM reset failure. */
        Log_Write(LOG_LEVEL_CRITICAL, "CM:Shire %d sending Ack to MM!\r\n", shire_id);

        /* Reset the thread boot counter */
        init_local_spinlock(&CM_Thread_Boot_Counter[shire_id], 0);

        /* Set the shire ID bit in the global CM shire boot mask */
        atomic_or_global_64(CM_BOOT_MASK_PTR, (1ULL << shire_id));

        /* Log boot message */
        Log_Write(LOG_LEVEL_CRITICAL, "CM:Shire %d booted up!\r\n", shire_id);
    }
    else if (booted_threads > thread_count)
    {
        /* Log boot error message */
        Log_Write(LOG_LEVEL_CRITICAL, "CM:Fatal Error:Thread boot count corrupted:%d\r\n",
            booted_threads);
    }

    /* Start the main processing loop */
    MM_To_CM_Iface_Main_Loop();
}
