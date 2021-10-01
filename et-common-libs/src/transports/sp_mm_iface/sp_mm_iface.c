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
static sp_mm_iface_cb_t SP_SQueue __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t SP_CQueue
    \brief Global SP to MM completion queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t SP_CQueue __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t MM_SQueue
    \brief Global MM to SP submission queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t MM_SQueue __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t MM_CQueue
    \brief Global MM to SP completion queue
    \warning Not thread safe!
*/
static sp_mm_iface_cb_t MM_CQueue __attribute__((aligned(64))) = {0};

#define SP_PLIC_TRIGGER_MASK    1

static inline int8_t notify(uint8_t target, int32_t hart_id_to_notify)
{
    uint64_t target_hart_msk = 0;
    int8_t status = 0;

    switch (target)
    {
        case    MM_CQ:
            /* SP polls for response in MM CQ, hence no notification required */
            break;
        case    SP_SQ:
        {
            /* Notify SP using PLIC */
            volatile uint32_t *const ipi_trigger_ptr =
                (volatile uint32_t *)(R_PU_TRG_MMIN_BASEADDR);
            *ipi_trigger_ptr = SP_PLIC_TRIGGER_MASK;
            break;
        }
        case    MM_SQ:
        case    SP_CQ:
        {
            if(hart_id_to_notify != SP_MM_IFACE_INTERRUPT_SP)
            {
                /* Notify requested HART in the Master Shire */
                target_hart_msk = (uint64_t)
                    (1 << (hart_id_to_notify - MM_BASE_HART_OFFSET));
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
*       flags   access method to r/w virtual queues between SP and MM
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Init(uint8_t flags)
{
    int8_t status = STATUS_SUCCESS;

    uint64_t temp64 = 0;

    temp64 = ((((uint64_t)SP2MM_SQ_SIZE) << 32) | ((uint64_t)SP2MM_SQ_BASE));
    ETSOC_Memory_Write_64(&temp64, (uint64_t*)&SP_SQueue, flags);
    temp64 = (((uint64_t)(SP2MM_CQ_SIZE << 32)) | SP2MM_CQ_BASE);
    ETSOC_Memory_Write_64(&temp64, (uint64_t*)&SP_CQueue, flags);

    status = VQ_Init(&SP_SQueue.vqueue, SP_SQueue.vqueue_base,
        SP_SQueue.vqueue_size, 0, sizeof(cmd_size_t), SP2MM_SQ_MEM_TYPE, flags);

    if(status == STATUS_SUCCESS)
    {
        /* Make a copy of SP SQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_Memory_Read_64((uint64_t*)&SP_SQueue.vqueue.circbuff_cb, &temp64, flags);
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)(void*)&SP_SQueue.circ_buff_local, flags);

        status = VQ_Init(&SP_CQueue.vqueue, SP_CQueue.vqueue_base,
            SP_CQueue.vqueue_size, 0, sizeof(cmd_size_t), SP2MM_CQ_MEM_TYPE, flags);
    }

    if(status == STATUS_SUCCESS)
    {
        /* Make a copy of SP CQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_Memory_Read_64((uint64_t*)&SP_CQueue.vqueue.circbuff_cb, &temp64, flags);
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)(void*)&SP_CQueue.circ_buff_local, flags);

        temp64 = ((((uint64_t)MM2SP_SQ_SIZE) << 32) | ((uint64_t)MM2SP_SQ_BASE));
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)&MM_SQueue, flags);
        temp64 = (((uint64_t)(MM2SP_CQ_SIZE << 32)) | ((uint64_t)MM2SP_CQ_BASE));
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)&MM_CQueue, flags);

        status = VQ_Init(&MM_SQueue.vqueue, MM_SQueue.vqueue_base,
            MM_SQueue.vqueue_size, 0, sizeof(cmd_size_t), MM2SP_SQ_MEM_TYPE, flags);
    }

    if(status == STATUS_SUCCESS)
    {
        /* Make a copy of MM SQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_Memory_Read_64((uint64_t*)&MM_SQueue.vqueue.circbuff_cb, &temp64, flags);
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)(void*)&MM_SQueue.circ_buff_local, flags);

        status = VQ_Init(&MM_CQueue.vqueue, MM_CQueue.vqueue_base,
            MM_CQueue.vqueue_size, 0, sizeof(cmd_size_t), MM2SP_CQ_MEM_TYPE, flags);
    }

    if(status == STATUS_SUCCESS)
    {
        /* Make a copy of MM CQ Circular Buffer CB in shared SRAM to global variable */
        ETSOC_Memory_Read_64((uint64_t*)&MM_CQueue.vqueue.circbuff_cb, &temp64, flags);
        ETSOC_Memory_Write_64(&temp64, (uint64_t*)(void*)&MM_CQueue.circ_buff_local, flags);

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
*       p_cmd   reference to command to push to target
*       cmd_size    size of command to be pushed to target
*       flags       access method to r/w virtual queues between SP and MM
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Push(uint8_t target, void* p_buff, uint32_t size, uint8_t flags)
{
    vq_cb_t *p_vq_cb = 0;
    int8_t status = 0;
    int32_t hart_id_to_notify;
    const struct dev_cmd_hdr_t *msg = p_buff;

    switch (target)
    {
        case    SP_SQ:
        {
            p_vq_cb = &SP_SQueue.vqueue;
            /* Set to -1, not applicable to interrupt directed to SP */
            hart_id_to_notify = SP_MM_IFACE_INTERRUPT_SP;
            break;
        }
        case    SP_CQ:
        {
            p_vq_cb = &SP_CQueue.vqueue;
            hart_id_to_notify = msg->issuing_hart_id;
            break;
        }
        case    MM_SQ:
        {
            p_vq_cb = &MM_SQueue.vqueue;
            hart_id_to_notify = SP2MM_CMD_NOTIFY_HART;
            break;
        }
        case    MM_CQ:
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

    if(p_vq_cb != 0)
    {
        status = VQ_Push(p_vq_cb, p_buff, size, flags);

        if(!status)
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
*       flags       access method to r/w virtual queues between SP and MM
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int32_t SP_MM_Iface_Pop(uint8_t target, void* rx_buff, uint8_t flags)
{
    sp_mm_iface_cb_t *p_iface_cb = 0;
    int32_t retval = 0;

    switch (target)
    {
        case    SP_SQ:
        {
            p_iface_cb = &SP_SQueue;
            break;
        }
        case    SP_CQ:
        {
            p_iface_cb = &SP_CQueue;
            break;
        }
        case    MM_SQ:
        {
            p_iface_cb = &MM_SQueue;
            break;
        }
        case    MM_CQ:
        {
            p_iface_cb = &MM_CQueue;
            break;
        }
        default:
        {
            p_iface_cb = 0;
        }
    }

    if(p_iface_cb != 0)
    {
        retval = VQ_Pop(&p_iface_cb->vqueue, rx_buff, flags);

        /* For now, just update the local copy for SP SQ */
        if(target == SP_SQ)
        {
            Circbuffer_Set_Tail(&p_iface_cb->circ_buff_local,
                Circbuffer_Get_Tail(p_iface_cb->vqueue.circbuff_cb, p_iface_cb->vqueue.flags),
                UNCACHED);
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
*       flags       access method to r/w virtual queues between SP and MM
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
bool SP_MM_Iface_Data_Available(uint8_t target, uint8_t flags)
{
    vq_cb_t *p_vq_cb = 0;
    bool result = false;

    switch (target)
    {
         case    SP_SQ:
        {
            p_vq_cb = &SP_SQueue.vqueue;
            break;
        }
        case    SP_CQ:
        {
            p_vq_cb = &SP_CQueue.vqueue;
            break;
        }
        case    MM_SQ:
        {
            p_vq_cb = &MM_SQueue.vqueue;
            break;
        }
        case    MM_CQ:
        {
            p_vq_cb = &MM_CQueue.vqueue;
            break;
        }
        default:
        {
            p_vq_cb = 0;
        }
    }

    if(p_vq_cb != 0)
    {
        result = VQ_Data_Avail(p_vq_cb, flags);
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
*       flags   access method to r/w virtual queues between SP and MM
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Verify_Tail(uint8_t target, uint8_t flags)
{
    sp_mm_iface_cb_t *p_iface_cb = 0;
    int8_t status = STATUS_SUCCESS;

    switch (target)
    {
        case    SP_SQ:
            p_iface_cb = &SP_SQueue;
            break;

        case    SP_CQ:
            p_iface_cb = &SP_CQueue;
            break;

        case    MM_SQ:
            p_iface_cb = &MM_SQueue;
            break;

        case    MM_CQ:
            p_iface_cb = &MM_CQueue;
            break;

        default:
            status = SP_MM_IFACE_ERROR_INVALID_TARGET;
            break;
    }

    /* Verify that the tail value read from shared memory is equal to previous head value */
    if(status == STATUS_SUCCESS)
    {
        uint64_t temp64 = 0;
        uint32_t temp32 = 0;

        ETSOC_Memory_Read_64((uint64_t*)&p_iface_cb->vqueue.circbuff_cb, &temp64, flags);
        ETSOC_Memory_Read_32(&p_iface_cb->vqueue.flags, &temp32, flags);

        uint64_t local_tail = Circbuffer_Get_Tail(&p_iface_cb->circ_buff_local, flags);
        uint64_t shared_tail = Circbuffer_Get_Tail((circ_buff_cb_t*)(uintptr_t)temp64, temp32);

        if(local_tail != shared_tail)
        {
            /* Fallback mechanism: use the cached copy of tail */
            VQ_Set_Tail_Offset(&p_iface_cb->vqueue, local_tail, flags);

            status = SP_MM_IFACE_ERROR_VQ_BAD_TAIL;
        }
    }

    return status;
}
