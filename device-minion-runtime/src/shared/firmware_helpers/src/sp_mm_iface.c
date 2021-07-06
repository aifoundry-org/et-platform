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
#include "sp_mm_iface.h"
#include "sp_mm_shared_config.h"
#include "sp_mm_comms_spec.h"
#include "device-common/esr_defines.h"
#include "hal_device.h"

typedef struct sp_iface_cb_ {
    uint32_t vqueue_base;
    uint32_t vqueue_size;
    vq_cb_t vqueue;
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
*       none
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Init(void)
{
    int8_t status = STATUS_SUCCESS;

#if defined(MASTER_MINION)

    uint64_t temp = 0;

    temp = ((((uint64_t)SP2MM_SQ_SIZE) << 32) | ((uint64_t)SP2MM_SQ_BASE));
    atomic_store_local_64((uint64_t*)&SP_SQueue, temp);
    temp = (((uint64_t)(SP2MM_CQ_SIZE << 32)) | SP2MM_CQ_BASE);
    atomic_store_local_64((uint64_t*)&SP_CQueue, temp);

    status = VQ_Init(&SP_SQueue.vqueue, SP_SQueue.vqueue_base,
                        SP_SQueue.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_SQ_MEM_TYPE);

    if (status == STATUS_SUCCESS)
    {
        status = VQ_Init(&SP_CQueue.vqueue, SP_CQueue.vqueue_base,
                            SP_CQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            SP2MM_CQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        temp = ((((uint64_t)MM2SP_SQ_SIZE) << 32) | ((uint64_t)MM2SP_SQ_BASE));
        atomic_store_local_64((uint64_t*)&MM_SQueue, temp);
        temp = (((uint64_t)(MM2SP_CQ_SIZE << 32)) | MM2SP_CQ_BASE);
        atomic_store_local_64((uint64_t*)&MM_CQueue, temp);

        status = VQ_Init(&MM_SQueue.vqueue, MM_SQueue.vqueue_base,
                            MM_SQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            MM2SP_SQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        status = VQ_Init(&MM_CQueue.vqueue, MM_CQueue.vqueue_base,
                            MM_CQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            MM2SP_CQ_MEM_TYPE);
    }

#elif defined(SERVICE_PROCESSOR_BL2)

    SP_SQueue.vqueue_base = SP2MM_SQ_BASE;
    SP_SQueue.vqueue_size = SP2MM_SQ_SIZE;
    SP_CQueue.vqueue_base = SP2MM_CQ_BASE;
    SP_CQueue.vqueue_size = SP2MM_CQ_SIZE;
    status = VQ_Init(&SP_SQueue.vqueue, SP_SQueue.vqueue_base,
                        SP_SQueue.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_SQ_MEM_TYPE);

    if (status == STATUS_SUCCESS)
    {
        status = VQ_Init(&SP_CQueue.vqueue, SP_CQueue.vqueue_base,
                            SP_CQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            SP2MM_CQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        MM_SQueue.vqueue_base = MM2SP_SQ_BASE;
        MM_SQueue.vqueue_size = MM2SP_SQ_SIZE;
        MM_CQueue.vqueue_base = MM2SP_CQ_BASE;
        MM_CQueue.vqueue_size = MM2SP_CQ_SIZE;
        status = VQ_Init(&MM_SQueue.vqueue, MM_SQueue.vqueue_base,
                            MM_SQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            MM2SP_SQ_MEM_TYPE);
    }

    if (status == STATUS_SUCCESS)
    {
        status = VQ_Init(&MM_CQueue.vqueue, MM_CQueue.vqueue_base,
                            MM_CQueue.vqueue_size, 0, sizeof(cmd_size_t),
                            MM2SP_CQ_MEM_TYPE);
    }
#endif

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
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int8_t SP_MM_Iface_Push(uint8_t target, void* p_buff, uint32_t size)
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
        status = VQ_Push(p_vq_cb, p_buff, size);

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
*       target  SP2MM_SQ, SP2MM_CQ, MM2SP_SQ, MM2SP_CQ
*       rx_buffer   Caller provided buffer to copy popped message
*
*   OUTPUTS
*
*       status      success or error code
*
***********************************************************************/
int32_t SP_MM_Iface_Pop(uint8_t target, void* rx_buff)
{
    vq_cb_t *p_vq_cb = 0;
    int32_t retval = 0;

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
        retval = VQ_Pop(p_vq_cb, rx_buff);
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
        result = VQ_Data_Avail(p_vq_cb);
    }

    return result;
}
