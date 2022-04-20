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
/*! \file cm_to_mm_iface.c
    \brief A C module that implements the CM to MM messaging data
    structures and interfaces.

    Public interfaces:
        CM_To_MM_Iface_Unicast_Send
        CM_To_MM_Save_Execution_Context
        CM_To_MM_Save_Kernel_Error

*/
/***********************************************************************/
#include <string.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <system/layout.h>
#include <transports/circbuff/circbuff.h>

#include "cm_to_mm_iface.h"
#include "mm_to_cm_iface.h"
#include "syscall_internal.h"

/********************/
/* Public Functions */
/********************/
int8_t CM_To_MM_Iface_Unicast_Send(
    uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message)
{
    int8_t status;
    circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                            cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);
    spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[cb_idx];

    /* Acquire the unicast buffer lock */
    acquire_global_spinlock(lock);

    status = Circbuffer_Push(cb, (const void *const)message, sizeof(*message), L2_SCP);

    if (status == STATUS_SUCCESS)
    {
        /* Send IPI to the required hart in Master Shire */
        syscall(SYSCALL_IPI_TRIGGER_INT, 1ull << ms_thread_id, MASTER_SHIRE, 0);
    }

    /* Release the unicast buffer lock */
    release_global_spinlock(lock);

    return status;
}

int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer, uint64_t type,
    uint64_t hart_id, const internal_execution_context_t *context)
{
    const uint64_t buffer_index = (hart_id < 2048U) ? hart_id : (hart_id - 32U);

    context_buffer[buffer_index].type = type;
    context_buffer[buffer_index].cycles = PMC_Get_Current_Cycles();
    context_buffer[buffer_index].hart_id = hart_id;
    context_buffer[buffer_index].scause = context->scause;
    context_buffer[buffer_index].sepc = context->sepc;
    context_buffer[buffer_index].stval = context->stval;
    context_buffer[buffer_index].sstatus = context->sstatus;

    /* Copy all the GPRs except x0 (hardwired to zero) */
    memcpy(context_buffer[buffer_index].gpr, &context->regs[1], sizeof(uint64_t) * 31);

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&context_buffer[buffer_index], sizeof(execution_context_t), to_L3)

    return 0;
}

int8_t CM_To_MM_Save_Kernel_Error(
    execution_context_t *context_buffer, uint64_t hart_id, uint64_t error_type, int64_t error_code)
{
    const uint64_t buffer_index = (hart_id < 2048U) ? hart_id : (hart_id - 32U);
    context_buffer[buffer_index].type = error_type;
    context_buffer[buffer_index].cycles = PMC_Get_Current_Cycles();
    context_buffer[buffer_index].hart_id = hart_id;
    context_buffer[buffer_index].user_error = error_code;

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&context_buffer[buffer_index], sizeof(execution_context_t), to_L3)

    return 0;
}
