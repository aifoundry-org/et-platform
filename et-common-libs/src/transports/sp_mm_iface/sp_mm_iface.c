/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file sp_mm_iface.c
    \brief A shared C module that implements interfaces needed for
    sp to mm interface

    Helpful design notes:
    SP assigned with:
        SP_SQ
        SP_CQ
    MM assigned with:
        MM_SQ
        MM_CQ
    For MM to SP comunications:
        MM pushes cmds to SP_SQ
        SP pops cmds from SP_SQ
        SP pushes rsps to SP_CQ
        MM pops rsps from SP_CQ
    For SP to MM comunications:
        SP pushes cmds to MM_SQ
        MM pops cmds from MM_SQ
        MM pushes rsps to MM_CQ
        SP pops rsps from MM_CQ

    Public interfaces:
        SP_MM_Iface_Init
        SP_MM_Iface_Push
        SP_MM_Iface_Pop
        SP_MM_Iface_Interrupt_Status
*/
/***********************************************************************/
#include "transports/sp_mm_iface/sp_mm_iface.h"
#include "transports/sp_mm_iface/sp_mm_shared_config.h"
#include "transports/sp_mm_iface/sp_mm_comms_spec.h"
#include "etsoc/isa/esr_defines.h"
#include "hwinc/hal_device.h"

/* Define this macro to enable logging in SP-MM interface transactions. */
#ifdef SP_MM_IFACE_ENABLE_LOGGING

#include "etsoc/common/log_internal.h"

#endif /* SP_MM_IFACE_ENABLE_LOGGING */

typedef struct sp_iface_cb_ {
    /* Shared copy globals */
    uint32_t vqueue_base;
    uint32_t vqueue_size;
    vq_cb_t vqueue;
    /* Local copy globals */
    circ_buff_cb_t circ_buff_local;
} sp_mm_iface_cb_t;

/*! \var iface_q_cb_t SP_SQueue
    \brief Global SP to MM submission queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t SP_SQueue __attribute__((aligned(64))) = { 0 };

/*! \var iface_q_cb_t SP_CQueue
    \brief Global SP to MM completion queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t SP_CQueue __attribute__((aligned(64))) = { 0 };

/*! \var iface_q_cb_t MM_SQueue
    \brief Global MM to SP submission queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t MM_SQueue __attribute__((aligned(64))) = { 0 };

/*! \var iface_q_cb_t MM_CQueue
    \brief Global MM to SP completion queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t MM_CQueue __attribute__((aligned(64))) = { 0 };

#define SP_PLIC_TRIGGER_MASK 1

static inline int8_t notify(uint8_t target, int32_t hart_id_to_notify)
{
    uint64_t target_hart_msk = 0;
    int8_t status = 0;

    switch (target)
    {
        case MM_CQ:
            /* SP polls for response in MM CQ, hence no notification required */
            break;
        case SP_SQ:
        {
            /* Notify SP using PLIC */
            volatile uint32_t *const ipi_trigger_ptr =
                (volatile uint32_t *)(R_PU_TRG_MMIN_BASEADDR);
            *ipi_trigger_ptr = SP_PLIC_TRIGGER_MASK;
            break;
        }
        case MM_SQ:
        case SP_CQ:
        {
            if (hart_id_to_notify != SP_MM_IFACE_INTERRUPT_SP)
            {
                /* Notify requested HART in the Master Shire */
                target_hart_msk = (uint64_t)(1 << (hart_id_to_notify - MM_BASE_HART_OFFSET));
                volatile uint64_t *const ipi_trigger_ptr =
                    (volatile uint64_t *)ESR_SHIRE(32, IPI_TRIGGER);
                *ipi_trigger_ptr = target_hart_msk;
            }
            break;
        }
        default:
        {
            status = VQ_ERROR_BAD_TARGET;
            break;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Init
*
*   DESCRIPTION
*
*       Initialize Service Processor (SP) to Master Minion (MM) runtime
*       interface. This is a shared API to be called by both SP and MM
*       runtimes.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Init(void)
{
    int8_t status = STATUS_SUCCESS;
    uint64_t temp64 = 0;

    temp64 = ((SP2MM_SQ_SIZE << 32) | SP2MM_SQ_BASE);
    ETSOC_RT_MEM_WRITE_64((uint64_t *)&SP_SQueue, temp64);
    temp64 = ((SP2MM_CQ_SIZE << 32) | SP2MM_CQ_BASE);
    ETSOC_RT_MEM_WRITE_64((uint64_t *)&SP_CQueue, temp64);

    /* TODO: This function call initializes circular buffer, hence break up SP and MM VQs init */
    status = VQ_Init(
        &SP_SQueue.vqueue, SP2MM_SQ_BASE, SP2MM_SQ_SIZE, 0, sizeof(cmd_size_t), SP2MM_SQ_MEM_TYPE);

    if (status == STATUS_SUCCESS)
    {
        /* Make a copy of SP SQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_RT_MEM_WRITE_64(&SP_SQueue.circ_buff_local.head_offset,
            Circbuffer_Get_Head((circ_buff_cb_t *)SP2MM_SQ_BASE, SP2MM_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&SP_SQueue.circ_buff_local.tail_offset,
            Circbuffer_Get_Tail((circ_buff_cb_t *)SP2MM_SQ_BASE, SP2MM_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&SP_SQueue.circ_buff_local.length,
            Circbuffer_Get_Length((circ_buff_cb_t *)SP2MM_SQ_BASE, SP2MM_SQ_MEM_TYPE));

        status = VQ_Init(&SP_CQueue.vqueue, SP2MM_CQ_BASE, SP2MM_CQ_SIZE, 0, sizeof(cmd_size_t),
            SP2MM_CQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Make a copy of SP CQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_RT_MEM_WRITE_64(&SP_CQueue.circ_buff_local.head_offset,
            Circbuffer_Get_Head((circ_buff_cb_t *)SP2MM_CQ_BASE, SP2MM_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&SP_CQueue.circ_buff_local.tail_offset,
            Circbuffer_Get_Tail((circ_buff_cb_t *)SP2MM_CQ_BASE, SP2MM_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&SP_CQueue.circ_buff_local.length,
            Circbuffer_Get_Length((circ_buff_cb_t *)SP2MM_CQ_BASE, SP2MM_SQ_MEM_TYPE));

        temp64 = ((MM2SP_SQ_SIZE << 32) | MM2SP_SQ_BASE);
        ETSOC_RT_MEM_WRITE_64((uint64_t *)&MM_SQueue, temp64);
        temp64 = ((MM2SP_CQ_SIZE << 32) | MM2SP_CQ_BASE);
        ETSOC_RT_MEM_WRITE_64((uint64_t *)&MM_CQueue, temp64);

        status = VQ_Init(&MM_SQueue.vqueue, MM2SP_SQ_BASE, MM2SP_SQ_SIZE, 0, sizeof(cmd_size_t),
            MM2SP_SQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Make a copy of MM SQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_RT_MEM_WRITE_64((uint64_t *)(void *)&MM_SQueue.circ_buff_local.head_offset,
            Circbuffer_Get_Head((circ_buff_cb_t *)MM2SP_SQ_BASE, MM2SP_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64((uint64_t *)(void *)&MM_SQueue.circ_buff_local.tail_offset,
            Circbuffer_Get_Tail((circ_buff_cb_t *)MM2SP_SQ_BASE, MM2SP_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64((uint64_t *)(void *)&MM_SQueue.circ_buff_local.length,
            Circbuffer_Get_Length((circ_buff_cb_t *)MM2SP_SQ_BASE, MM2SP_SQ_MEM_TYPE));

        status = VQ_Init(&MM_CQueue.vqueue, MM2SP_CQ_BASE, MM2SP_CQ_SIZE, 0, sizeof(cmd_size_t),
            MM2SP_CQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Make a copy of MM CQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_RT_MEM_WRITE_64(&MM_CQueue.circ_buff_local.head_offset,
            Circbuffer_Get_Head((circ_buff_cb_t *)MM2SP_CQ_BASE, MM2SP_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&MM_CQueue.circ_buff_local.tail_offset,
            Circbuffer_Get_Tail((circ_buff_cb_t *)MM2SP_CQ_BASE, MM2SP_SQ_MEM_TYPE));
        ETSOC_RT_MEM_WRITE_64(&MM_CQueue.circ_buff_local.length,
            Circbuffer_Get_Length((circ_buff_cb_t *)MM2SP_CQ_BASE, MM2SP_SQ_MEM_TYPE));
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Push
*
*   DESCRIPTION
*
*       Push command to SP to MM interface target specified by caller
*
*   INPUTS
*
*       target  SP2MM_SQ, SP2MM_CQ, MM2SP_SQ, MM2SP_CQ
*       p_buff   reference to command to push to target
*       size    size of command to be pushed to target
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Push(uint8_t target, const void *p_buff, uint32_t size)
{
    vq_cb_t *p_vq_cb = 0;
    int8_t status = 0;
    int32_t hart_id_to_notify;
    const struct dev_cmd_hdr_t *msg = p_buff;

    switch (target)
    {
        case SP_SQ:
        {
            p_vq_cb = &SP_SQueue.vqueue;
            /* Set to -1, not applicable to interrupt directed to SP */
            hart_id_to_notify = SP_MM_IFACE_INTERRUPT_SP;
            break;
        }
        case SP_CQ:
        {
            p_vq_cb = &SP_CQueue.vqueue;
            hart_id_to_notify = msg->issuing_hart_id;
            break;
        }
        case MM_SQ:
        {
            p_vq_cb = &MM_SQueue.vqueue;
            hart_id_to_notify = SP2MM_CMD_NOTIFY_HART;
            break;
        }
        case MM_CQ:
        {
            p_vq_cb = &MM_CQueue.vqueue;
            /* Set to -1, not applicable to interrupt directed to SP */
            hart_id_to_notify = SP_MM_IFACE_INTERRUPT_SP;
            break;
        }
        default:
        {
            p_vq_cb = 0;
            break;
        }
    }

    if (p_vq_cb != 0)
    {
#ifdef SP_MM_IFACE_ENABLE_LOGGING
        circ_buff_cb_t *circ_buff_ptr =
            (circ_buff_cb_t *)(uintptr_t)ETSOC_RT_MEM_READ_64((uint64_t *)&p_vq_cb->circbuff_cb);

        Log_Write(LOG_LEVEL_INFO, "%s%s%p%s%ld%s%ld%s%ld%s", LOG_FROM,
            "SP_MM_Iface_Push:before_push:target_circ_buff:", circ_buff_ptr,
            ":head:", circ_buff_ptr->head_offset, ":tail:", circ_buff_ptr->tail_offset,
            ":length:", circ_buff_ptr->length, "\r\n");
#endif /* SP_MM_IFACE_ENABLE_LOGGING */

        status = VQ_Push(p_vq_cb, p_buff, size);

#ifdef SP_MM_IFACE_ENABLE_LOGGING
        circ_buff_ptr =
            (circ_buff_cb_t *)(uintptr_t)ETSOC_RT_MEM_READ_64((uint64_t *)&p_vq_cb->circbuff_cb);

        Log_Write(LOG_LEVEL_INFO, "%s%s%p%s%ld%s%ld%s%ld%s", LOG_FROM,
            "SP_MM_Iface_Push:after_push:target_circ_buff:", circ_buff_ptr,
            ":head:", circ_buff_ptr->head_offset, ":tail:", circ_buff_ptr->tail_offset,
            ":length:", circ_buff_ptr->length, "\r\n");
#endif /* SP_MM_IFACE_ENABLE_LOGGING */

        if (!status)
        {
            status = notify(target, hart_id_to_notify);
        }
    }

    return status;
}
/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Get_VQ_Base_Addr
*
*   DESCRIPTION
*
*       Provides pointer to virtual queue control block
*
*   INPUTS
*
*       target      SP2MM_SQ, SP2MM_CQ, MM2SP_SQ, MM2SP_CQ
*
*   OUTPUTS
*
*       vq_cb_t     Pointer to VQ or NULL
*
***********************************************************************/
vq_cb_t *SP_MM_Iface_Get_VQ_Base_Addr(uint8_t target)
{
    vq_cb_t *vqueue;
    switch (target)
    {
        case SP_SQ:
        {
            vqueue = &SP_SQueue.vqueue;
            break;
        }
        case SP_CQ:
        {
            vqueue = &SP_CQueue.vqueue;
            break;
        }
        case MM_SQ:
        {
            vqueue = &MM_SQueue.vqueue;
            break;
        }
        case MM_CQ:
        {
            vqueue = &MM_CQueue.vqueue;
            break;
        }
        default:
        {
            vqueue = NULL;
            break;
        }
    }

    return vqueue;
}
/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Pop
*
*   DESCRIPTION
*
*       Pop message from SP to MM interface target specified by caller
*
*   INPUTS
*
*       target      SP2MM_SQ, SP2MM_CQ, MM2SP_SQ, MM2SP_CQ
*       rx_buffer   Caller provided buffer to copy popped message
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int32_t SP_MM_Iface_Pop(uint8_t target, void *rx_buff)
{
    sp_mm_iface_cb_t *p_iface_cb = 0;
    int32_t retval = 0;

    switch (target)
    {
        case SP_SQ:
        {
            p_iface_cb = &SP_SQueue;
            break;
        }
        case SP_CQ:
        {
            p_iface_cb = &SP_CQueue;
            break;
        }
        case MM_SQ:
        {
            p_iface_cb = &MM_SQueue;
            break;
        }
        case MM_CQ:
        {
            p_iface_cb = &MM_CQueue;
            break;
        }
        default:
        {
            p_iface_cb = 0;
        }
    }

    if (p_iface_cb != 0)
    {
        retval = VQ_Pop(&p_iface_cb->vqueue, rx_buff);

        /* For now, just update the local copy for SP SQ */
        if (target == SP_SQ)
        {
            ETSOC_RT_MEM_WRITE_64(&p_iface_cb->circ_buff_local.tail_offset,
                Circbuffer_Get_Tail((circ_buff_cb_t *)(uintptr_t)ETSOC_RT_MEM_READ_64(
                                        (uint64_t *)(uintptr_t)&p_iface_cb->vqueue.circbuff_cb),
                    ETSOC_RT_MEM_READ_32(&p_iface_cb->vqueue.flags)));
        }
    }

    return retval;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Data_Available
*
*   DESCRIPTION
*
*       This API checks if data is available to process in the target
*       SP to MM interface
*
*   INPUTS
*
*       target      target enum
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
bool SP_MM_Iface_Data_Available(uint8_t target)
{
    vq_cb_t *p_vq_cb = 0;
    bool result = false;

    switch (target)
    {
        case SP_SQ:
        {
            p_vq_cb = &SP_SQueue.vqueue;
            break;
        }
        case SP_CQ:
        {
            p_vq_cb = &SP_CQueue.vqueue;
            break;
        }
        case MM_SQ:
        {
            p_vq_cb = &MM_SQueue.vqueue;
            break;
        }
        case MM_CQ:
        {
            p_vq_cb = &MM_CQueue.vqueue;
            break;
        }
        default:
        {
            p_vq_cb = 0;
        }
    }

    if (p_vq_cb != 0)
    {
        result = VQ_Data_Avail(p_vq_cb);
    }

    return result;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_MM_Iface_Verify_Tail
*
*   DESCRIPTION
*
*       Function used to verify the tail offset of a SP-MM VQ.
*
*   INPUTS
*
*       target  SP2MM_SQ, SP2MM_CQ, MM2SP_SQ, MM2SP_CQ
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Verify_Tail(uint8_t target)
{
    sp_mm_iface_cb_t *p_iface_cb = 0;
    int8_t status = STATUS_SUCCESS;

    switch (target)
    {
        case SP_SQ:
            p_iface_cb = &SP_SQueue;
            break;

        case SP_CQ:
            p_iface_cb = &SP_CQueue;
            break;

        case MM_SQ:
            p_iface_cb = &MM_SQueue;
            break;

        case MM_CQ:
            p_iface_cb = &MM_CQueue;
            break;

        default:
            status = SP_MM_IFACE_ERROR_INVALID_TARGET;
            break;
    }

    /* Verify that the tail value read from shared memory is equal to previous head value */
    if (status == STATUS_SUCCESS)
    {
        uint64_t local_tail = ETSOC_RT_MEM_READ_64(&p_iface_cb->circ_buff_local.tail_offset);
        uint64_t shared_tail =
            Circbuffer_Get_Tail((circ_buff_cb_t *)(uintptr_t)ETSOC_RT_MEM_READ_64(
                                    (uint64_t *)&p_iface_cb->vqueue.circbuff_cb),
                ETSOC_RT_MEM_READ_32(&p_iface_cb->vqueue.flags));

        if (local_tail != shared_tail)
        {
            /* Fallback mechanism: use the cached copy of tail */
            VQ_Set_Tail_Offset(&p_iface_cb->vqueue, local_tail);

            status = SP_MM_IFACE_ERROR_VQ_BAD_TAIL;
        }
    }

    return status;
}
