/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file extended_interrupt.c
    \brief A C module that handles ET specific interrupts.

*/
/***********************************************************************/
/* cm_rt_svcs */
#include <etsoc/isa/macros.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/hart.h>
#include <etsoc/common/common_defs.h>

/* FW common headers */
#include "error_codes.h"

/* cm specific headers */
#include "cm_to_mm_iface.h"
#include "kernel.h"
#include "log.h"
#include "trace.h"

void extended_interrupt(uint64_t scause, uint64_t sepc, uint64_t stval, const uint64_t *regs);

void extended_interrupt(uint64_t scause, uint64_t sepc, uint64_t stval, const uint64_t *regs)
{
    uint64_t sstatus;
    (void)regs;
    CSR_READ_SSTATUS(sstatus)

    /* Check if bus error interrupt */
    if ((scause & 0xFF) == BUS_ERROR_INTERRUPT)
    {
        /* Clear the bus error interrupt */
        asm volatile("csrc sip, %0" : : "r"(1 << BUS_ERROR_INTERRUPT));

        /* Check if bus error interupt was generated from U-Mode.
           The SPP bit would be 0 if interrupt was generated from U-Mode, otherwise 1. */
        if (!(sstatus & SSTATUS_SPP))
        {
            /* Set the kernel bus error mask for current HART which took bus error interrupt. */
            kernel_info_set_local_bus_error_mask(
                get_shire_id(), get_hart_id() & (HARTS_PER_SHIRE - 1));

            Log_Write(LOG_LEVEL_ERROR,
                "CM:U-mode:Bus error:scause: %lx sepc: %lx stval: %lx sstatus: %lx\n", scause, sepc,
                stval, sstatus);

            /* Evict S-mode and U-mode Trace buffers to L3. */
            Trace_Evict_CM_Buffer();
            Trace_Evict_UMode_Buffer();

            /* Return from kernel */
            return_from_kernel(0, KERNEL_RETURN_BUS_ERROR);
        }
        else /* S-mode */
        {
            Log_Write(LOG_LEVEL_CRITICAL,
                "CM:S-mode:Bus error interrupt:scause: %lx sepc: %lx stval: %lx sstatus: %lx\n",
                scause, sepc, stval, sstatus);

            cm_to_mm_message_fw_error_t message = { .header.id = CM_TO_MM_MESSAGE_ID_FW_ERROR,
                .hart_id = get_hart_id(),
                .error_code = CM_FW_BUS_ERROR_RECEIVED };

            /* Send error message to dispatcher (Master shire Hart 0) */
            CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
                CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (cm_iface_message_t *)&message);

            /* Evict S-mode Trace buffer to L3. */
            Trace_Evict_CM_Buffer();

            /* Should only return from kernel if the kernel was launched on this hart.
            This is only required if S-Mode bus error occurred while handling the U-Mode bus error */
            if (kernel_info_has_thread_launched(
                    get_shire_id(), get_hart_id() & (HARTS_PER_SHIRE - 1)))
            {
                return_from_kernel(0, KERNEL_RETURN_BUS_ERROR);
            }
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL,
            "CM:Unhandled ET specific interrupt:scause: %lx sepc: %lx stval: %lx sstatus: %lx\n",
            scause, sepc, stval, sstatus);
    }
}
