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

    Public interfaces:
        SP_MM_Iface_Init
        SP_MM_Iface_Push
        SP_MM_Iface_Pop
        SP_MM_Iface_Interrupt_Status
*/
/***********************************************************************/
#include "sp_mm_iface.h"
#include "sp_mm_shared_config.h"
#include "device-common/esr_defines.h"
#include "hal_device.h"
#include "vq.h"

typedef struct sp_iface_cb_ {
    uint32_t vqueue_base;
    uint32_t vqueue_size;
    vq_cb_t vqueue;
} sp_iface_cb_t;

/*! \var iface_q_cb_t SP_MM_SQ
    \brief Global SP to MM submission queue
    \warning Not thread safe!
*/
static sp_iface_cb_t SP_MM_SQ __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t SP_MM_CQ
    \brief Global SP to MM completion queue
    \warning Not thread safe!
*/
static sp_iface_cb_t SP_MM_CQ __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t MM_SP_SQ
    \brief Global MM to SP submission queue
    \warning Not thread safe!
*/
static sp_iface_cb_t MM_SP_SQ __attribute__((aligned(64))) = {0};

/*! \var iface_q_cb_t MM_SP_CQ
    \brief Global MM to SP completion queue
    \warning Not thread safe!
*/
static sp_iface_cb_t MM_SP_CQ __attribute__((aligned(64))) = {0};

static void notify(uint8_t target);

static void notify(uint8_t target)
{
    switch (target)
    {
        case    SP_SQ: /* MM notifies SP - push to SP SQ */
        case    MM_CQ: /* MM notifies SP - push to SP CQ */
        {
            /* Send IPI to Shire 32 HART 0 */
            volatile uint64_t *const ipi_trigger_ptr =
                (volatile uint64_t *)ESR_SHIRE(32, IPI_TRIGGER);
            *ipi_trigger_ptr = 1;
            break;
        }
        case    SP_CQ: /* SP notifies MM - push to SP CQ */
        case    MM_SQ: /* SP notifies MM - push to MM SQ */
        {
             /* Notify SP using PLIC */
            volatile uint32_t *const ipi_trigger_ptr =
                (volatile uint32_t *)(R_PU_TRG_MMIN_BASEADDR);
            *ipi_trigger_ptr = 1;
            break;
        }
    }
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
    int8_t status = 0;

#if defined(MASTER_MINION)

    uint64_t temp = 0;

    temp = ((((uint64_t)SP2MM_SQ_SIZE) << 32) | ((uint64_t)SP2MM_SQ_BASE));
    atomic_store_local_64((uint64_t*)&SP_MM_SQ, temp);
    temp = (((uint64_t)(SP2MM_CQ_SIZE << 32)) | SP2MM_CQ_BASE);
    atomic_store_local_64((uint64_t*)&SP_MM_CQ, temp);

    status = VQ_Init(&SP_MM_SQ.vqueue, SP_MM_SQ.vqueue_base,
                        SP_MM_SQ.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_SQ_MEM_TYPE);
    status = VQ_Init(&SP_MM_CQ.vqueue, SP_MM_CQ.vqueue_base,
                        SP_MM_CQ.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_CQ_MEM_TYPE);


    temp = ((((uint64_t)MM2SP_SQ_SIZE) << 32) | ((uint64_t)MM2SP_SQ_BASE));
    atomic_store_local_64((uint64_t*)&MM_SP_SQ, temp);
    temp = (((uint64_t)(MM2SP_CQ_SIZE << 32)) | MM2SP_CQ_BASE);
    atomic_store_local_64((uint64_t*)&MM_SP_CQ, temp);

    status = VQ_Init(&MM_SP_SQ.vqueue, MM_SP_SQ.vqueue_base,
                        MM_SP_SQ.vqueue_size, 0, sizeof(cmd_size_t),
                        MM2SP_SQ_MEM_TYPE);
    status = VQ_Init(&MM_SP_CQ.vqueue, MM_SP_CQ.vqueue_base,
                        MM_SP_CQ.vqueue_size, 0, sizeof(cmd_size_t),
                        MM2SP_CQ_MEM_TYPE);

#elif defined(SERVICE_PROCESSOR_BL2)

    SP_MM_SQ.vqueue_base = SP2MM_SQ_BASE;
    SP_MM_SQ.vqueue_size = SP2MM_SQ_SIZE;
    SP_MM_CQ.vqueue_base = SP2MM_CQ_BASE;
    SP_MM_CQ.vqueue_size = SP2MM_CQ_SIZE;
    status = VQ_Init(&SP_MM_SQ.vqueue, SP_MM_SQ.vqueue_base,
                        SP_MM_SQ.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_SQ_MEM_TYPE);
    status = VQ_Init(&SP_MM_CQ.vqueue, SP_MM_CQ.vqueue_base,
                        SP_MM_CQ.vqueue_size, 0, sizeof(cmd_size_t),
                        SP2MM_CQ_MEM_TYPE);

    MM_SP_SQ.vqueue_base = MM2SP_SQ_BASE;
    MM_SP_SQ.vqueue_size = MM2SP_SQ_SIZE;
    MM_SP_CQ.vqueue_base = MM2SP_CQ_BASE;
    MM_SP_CQ.vqueue_size = MM2SP_CQ_SIZE;
    status = VQ_Init(&MM_SP_SQ.vqueue, MM_SP_SQ.vqueue_base,
                        MM_SP_SQ.vqueue_size, 0, sizeof(cmd_size_t),
                        MM2SP_SQ_MEM_TYPE);
    status = VQ_Init(&MM_SP_CQ.vqueue, MM_SP_CQ.vqueue_base,
                        MM_SP_CQ.vqueue_size, 0, sizeof(cmd_size_t),
                        MM2SP_CQ_MEM_TYPE);
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
int8_t SP_MM_Iface_Push(uint8_t target, void* p_cmd,
    uint32_t cmd_size)
{
    vq_cb_t *p_vq_cb = 0;
    int8_t status = 0;

    switch (target)
    {
        case    SP_SQ:
        {
            p_vq_cb = &SP_MM_SQ.vqueue;
            break;
        }
        case    SP_CQ:
        {
            p_vq_cb = &SP_MM_CQ.vqueue;
            break;
        }
        case    MM_SQ:
        {
            p_vq_cb = &MM_SP_SQ.vqueue;
            break;
        }
        case    MM_CQ:
        {
            p_vq_cb = &MM_SP_CQ.vqueue;
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
        status = VQ_Push(p_vq_cb, p_cmd, cmd_size);

        if(!status)
        {
            notify(target);
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
int8_t SP_MM_Iface_Pop(uint8_t target, void* rx_buff)
{
    vq_cb_t *p_vq_cb = 0;
    int32_t retval = 0;

    switch (target)
    {
        case    SP_SQ:
        {
            p_vq_cb = &SP_MM_SQ.vqueue;
            break;
        }
        case    SP_CQ:
        {
            p_vq_cb = &SP_MM_CQ.vqueue;
            break;
        }
        case    MM_SQ:
        {
            p_vq_cb = &MM_SP_SQ.vqueue;
            break;
        }
        case    MM_CQ:
        {
            p_vq_cb = &MM_SP_CQ.vqueue;
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

    return ((int8_t) retval);
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
            p_vq_cb = &SP_MM_SQ.vqueue;
            break;
        }
        case    SP_CQ:
        {
            p_vq_cb = &SP_MM_CQ.vqueue;
            break;
        }
        case    MM_SQ:
        {
            p_vq_cb = &MM_SP_SQ.vqueue;
            break;
        }
        case    MM_CQ:
        {
            p_vq_cb = &MM_SP_CQ.vqueue;
            break;
        }
        default:
        {
            p_vq_cb = 0;
        }
    }

    result = VQ_Data_Avail(p_vq_cb);

    return result;
}

