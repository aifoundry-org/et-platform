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
/*! \file cm_iface.c
    \brief A C module that implements the interfaces needed to
    offload work, and handle completion and other error/exception
    related events from compute minions

    Public interfaces:
        CM_Iface_Init
        CM_Iface_Multicast_Send
        CM_Iface_Unicast_Receive
*/
/***********************************************************************/
#include <stdbool.h>

/* mm_rt_svcs */
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <transports/mm_cm_iface/broadcast.h>
#include <transports/circbuff/circbuff.h>

/* mm specific headers */
#include "config/mm_config.h"
#include "services/log.h"
#include "services/cm_iface.h"
#include "services/sw_timer.h"

/* mm_rt_helpers */
#include "layout.h"
#include "syscall_internal.h"
#include "cm_mm_defines.h"

/* master -> worker */
#define mm_to_cm_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define mm_to_cm_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

/*! \typedef mm_cm_iface_cb_t
    \brief MM to CM Iface Control Block structure.
*/
typedef struct mm_cm_iface_cb {
    spinlock_t mm_to_cm_broadcast_lock;
    uint32_t   timeout_flag;
    uint8_t    sw_timer_idx;
} mm_cm_iface_cb_t;

/*! \var mm_cm_iface_cb_t MM_CM_CB
    \brief Global MM to CM Iface Control Block
    \warning Not thread safe!
*/
static mm_cm_iface_cb_t MM_CM_CB __attribute__((aligned(64))) = { 0 };

/*! \var uint32_t MM_CM_Broadcast_Last_Number
    \brief Global MM to CM Iface message last number
    \warning Not thread safe!
*/
static uint32_t MM_CM_Broadcast_Last_Number __attribute__((aligned(64))) = 1;


static void mm_to_cm_iface_multicast_timeout_cb(uint8_t arg);

static inline int64_t broadcast_ipi_trigger
    (uint64_t dest_shire_mask, uint64_t dest_hart_mask)
{
    const uint64_t broadcast_parameters = broadcast_encode_parameters(
        ESR_SHIRE_IPI_TRIGGER_PROT, ESR_SHIRE_REGION,
        ESR_SHIRE_IPI_TRIGGER_REGNO);

    /* Broadcast dest_hart_mask to IPI_TRIGGER ESR in all shires
    in dest_shire_mask */
    return syscall(SYSCALL_BROADCAST_INT, dest_hart_mask,
        dest_shire_mask, broadcast_parameters);
}

static void mm_to_cm_iface_multicast_timeout_cb(uint8_t thread_id)
{
    /* Free the registered SW Timeout slot */
    SW_Timer_Cancel_Timeout(atomic_load_local_8(&MM_CM_CB.sw_timer_idx));

    /* Set the timeout flag */
    atomic_store_local_32(&MM_CM_CB.timeout_flag, 1);

    /* Trigger IPI to respective hart */
    syscall(SYSCALL_IPI_TRIGGER_INT, (1ULL << thread_id), MASTER_SHIRE, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       CM_Iface_Init
*
*   DESCRIPTION
*
*       Initializes message buffer. Should only be called by master minion.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
int8_t CM_Iface_Init(void)
{
    int8_t status = 0;

    init_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock, 0);
    atomic_store_local_32(&MM_CM_CB.timeout_flag, 0);

    /* Master->worker broadcast message number and id */
    atomic_store_global_8
        (&mm_to_cm_broadcast_message_buffer_ptr->header.number, 0);
    atomic_store_global_8
        (&mm_to_cm_broadcast_message_buffer_ptr->header.id,
        MM_TO_CM_MESSAGE_ID_NONE);

    /* CM to MM Unicast Circularbuffer control blocks */
    for (uint32_t i = 0; i < (1 + MAX_SIMULTANEOUS_KERNELS); i++)
    {
        circ_buff_cb_t *cb =
            (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR
            + i * CM_MM_IFACE_CIRCBUFFER_SIZE);

        spinlock_t *lock =
            &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[i];

        init_global_spinlock(lock, 0);

        status = Circbuffer_Init(cb,
        (uint32_t)(CM_MM_IFACE_CIRCBUFFER_SIZE - sizeof(circ_buff_cb_t)),
        L2_SCP);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CM_Iface_Multicast_Send
*
*   DESCRIPTION
*
*       Broadcasts a message to all worker HARTS in all Shires in
*       dest_shire_mask. Can be called from multiple threads from
*       Master Shire. Blocks until all the receivers have ACK'd.
*
*   INPUTS
*
*       arg   timeout
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
int8_t CM_Iface_Multicast_Send(uint64_t dest_shire_mask,
        cm_iface_message_t *const message)
{
    int8_t sw_timer_idx;
    int8_t status = 0;
    uint8_t thread_id = get_hart_id() & (HARTS_PER_SHIRE - 1);
    uint32_t timeout_flag = 0;
    uint64_t sip;
    broadcast_message_ctrl_t msg_control;

    Log_Write(LOG_LEVEL_DEBUG,
        "CM_Iface_Multicast_Send:Sending multicast msg\r\n");

    /* Verify the shire mask */
    if(dest_shire_mask == 0)
    {
        return CM_IFACE_MULTICAST_INVLD_SHIRE_MASK;
    }

    acquire_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock);

    /* Create timeout for MM->CM multicast complete */
    sw_timer_idx = SW_Timer_Create_Timeout(
        &mm_to_cm_iface_multicast_timeout_cb, thread_id,
        TIMEOUT_MM_CM_MSG(5));

    if(sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "MM->CM: Unable to register Multicast timeout!\r\n");
        status = -1;
    }
    else
    {
        /* Save the SW timer index in global CB */
        atomic_store_local_8(&MM_CM_CB.sw_timer_idx,
            (uint8_t)sw_timer_idx);

        /* Check for overflow */
        atomic_compare_and_exchange_local_32(&MM_CM_Broadcast_Last_Number, 255, 1);

        /* Update the message number */
        message->header.number = (uint8_t)atomic_add_local_32(&MM_CM_Broadcast_Last_Number, 1);

        /* Configure broadcast message control data */
        msg_control.shire_count = (uint32_t)__builtin_popcountll(dest_shire_mask);
        msg_control.sender_thread_id = thread_id;
        atomic_store_global_64(&mm_to_cm_broadcast_message_ctrl_ptr->raw_u64,
            msg_control.raw_u64);

        /* Copy message to shared global buffer */
        ETSOC_MEM_COPY_AND_EVICT(mm_to_cm_broadcast_message_buffer_ptr, message,
            sizeof(*message), to_L3)

        /* Send IPI to receivers. Upper 32 Threads of Shire 32 also run Worker FW */
        broadcast_ipi_trigger(dest_shire_mask & 0xFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu);
        if (dest_shire_mask & (1ULL << MASTER_SHIRE))
        {
            syscall(SYSCALL_IPI_TRIGGER_INT, 0xFFFFFFFF00000000u, MASTER_SHIRE, 0);
        }

        /* Wait until all the receiver Shires have ACK'd. 1 IPI for all the shires.
        Then it's safe to send another broadcast message.
        Also, wait until timeout is expired. */
        do {
            /* Wait for an interrupt */
            asm volatile("wfi");

            /* Read pending interrupts */
            SUPERVISOR_PENDING_INTERRUPTS(sip);

            /* We are only interested in IPIs */
            if(!(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)))
            {
                continue;
            }

            /* Clear IPI pending interrupt */
            asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Read the global timeout flag to see for MM->CM message timeout */
            timeout_flag =
                atomic_compare_and_exchange_local_32(&MM_CM_CB.timeout_flag, 1, 0);

            /* Continue to wait for IPI until we shire count is 0 or timeout has occured */
        } while ((atomic_load_global_32(&mm_to_cm_broadcast_message_ctrl_ptr->shire_count) != 0)
                && (timeout_flag == 0));

        /* Check for timeout status */
        if(timeout_flag != 0)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "MM->CM Multicast timeout abort!\r\n");
            status = -1;
        }
        else
        {
            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
        }
    }

    release_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CM_Iface_Unicast_Receive
*
*   DESCRIPTION
*
*       Function to receive any message from CM to MM unicast buffer.
*
*   INPUTS
*
*       cb_idx    Index of the unicast buffer
*       message   Pointer to message buffer
*
*   OUTPUTS
*
*       int8_t    status success or failure
*
***********************************************************************/
int8_t CM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message)
{
    int8_t status;

    circ_buff_cb_t *cb =
        (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
        cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);

    /* Pop the command from circular buffer */
    status = Circbuffer_Pop(cb, message, sizeof(*message), L2_SCP);

    return status;
}
