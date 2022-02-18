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
************************************************************************/
/***********************************************************************/
/*! \file spw.c
    \brief A C module that implements the Service Processor Queue Worker's
    public and private interfaces.
    Public interfaces:
        SPW_Launch

*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/riscv_encoding.h>

/* mm specific headers */
#include "workers/spw.h"
#include "services/log.h"
#include "services/sp_iface.h"

/************************************************************************
*
*   FUNCTION
*
*       SPW_Launch
*
*   DESCRIPTION
*
*       Launch a Service Processor Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the Service Processor worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SPW_Launch(uint32_t hart_id)
{
    uint64_t sip;
    uint64_t tail_prev;
    void *spw_shared_mem;
    circ_buff_cb_t spw_circ_buff_cached __attribute__((aligned(64)));
    vq_cb_t spw_vq_cached;

    Log_Write(LOG_LEVEL_INFO, "SPW:launched on H%d\r\n", hart_id);

    /* Declare a pointer to the VQ control block that is pointing to the VQ attributes in
    global memory and the actual Circular Buffer CB in SRAM (which is shared with host) */
    vq_cb_t *spw_vq_shared = SP_MM_Iface_Get_VQ_Base_Addr(MM_SQ);

    /* Make a copy of the VQ CB in cached DRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Local_Atomic(
        (void *)spw_vq_shared, (void *)&spw_vq_cached, sizeof(spw_vq_cached));

    /* Make a copy of the Circular Buffer CB in shared SRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Uncacheable((void *)spw_vq_cached.circbuff_cb, (void *)&spw_circ_buff_cached,
        sizeof(spw_circ_buff_cached));

    /* Save the shared memory pointer */
    spw_shared_mem = (void *)spw_vq_cached.circbuff_cb->buffer_ptr;

    /* Verify that the head pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&spw_circ_buff_cached.head_offset, 8) &&
            IS_ALIGNED(&spw_vq_cached.circbuff_cb->head_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SPW:SQ HEAD not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_SPW_ERROR, MM_SQ_BUFFER_ALIGNMENT_ERROR);
    }

    /* Verify that the tail pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&spw_circ_buff_cached.tail_offset, 8) &&
            IS_ALIGNED(&spw_vq_cached.circbuff_cb->tail_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SPW:SQ tail not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_SPW_ERROR, MM_SQ_BUFFER_ALIGNMENT_ERROR);
    }

    /* Update the local VQ CB to point to the cached L1 stack variable */
    spw_vq_cached.circbuff_cb = &spw_circ_buff_cached;

    Log_Write(LOG_LEVEL_INFO, "SPW:H%d\r\n", hart_id);

    while (1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        /* We are only interested in IPIs */
        if (!(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)))
        {
            continue;
        }

        /* Clear IPI pending interrupt */
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        Log_Write(LOG_LEVEL_DEBUG, "SPW:IPI received!\r\n");

        /* Get the cached tail pointer */
        tail_prev = VQ_Get_Tail_Offset(&spw_vq_cached);

        /* Refresh the cached VQ CB - Get updated head and tail values */
        VQ_Get_Head_And_Tail(spw_vq_shared, &spw_vq_cached);

        /* Verify that the tail value read from memory is equal to previous tail value */
        if (tail_prev != VQ_Get_Tail_Offset(&spw_vq_cached))
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SPW:FATAL_ERROR:Tail Mismatch:Cached: %ld, Shared Memory: %ld Using cached value as fallback mechanism\r\n",
                tail_prev, VQ_Get_Tail_Offset(&spw_vq_cached));

            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_SPW_ERROR, MM_SQ_PROCESSING_ERROR);

            spw_vq_cached.circbuff_cb->tail_offset = tail_prev;
        }

        /* Calculate the total number of bytes available in the VQ */
        uint64_t vq_used_space = VQ_Get_Used_Space(&spw_vq_cached, CIRCBUFF_FLAG_NO_READ);

        /* Process commands until there is no more data in VQ */
        while (vq_used_space)
        {
            /* Process the pending commands */
            SP_Iface_Processing(&spw_vq_cached, spw_vq_shared, spw_shared_mem, vq_used_space);

            /* Refresh the cached VQ CB - Get updated head and tail values */
            VQ_Get_Head_And_Tail(spw_vq_shared, &spw_vq_cached);

            /* Re-calculate the total number of bytes available in the cached VQ copy */
            vq_used_space = VQ_Get_Used_Space(&spw_vq_cached, CIRCBUFF_FLAG_NO_READ);
        }

    } /* loop forever */

    /* will not return */
}
