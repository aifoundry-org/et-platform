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
/***********************************************************************/
/*! \file cw.c
    \brief A C module that implements the compute worker related
    public and private interfaces. This component provides interfaces
    to other components in the Master Minion runtime for initialization,
    and management, of compute shires available on the device. Additionally
    a helper fn is provided by this component to handle messages from
    compute minion S mode firmware.
    It implements the interfaces listed below

    Public interfaces:
        CW_Init
        CW_Process_CM_SMode_Messages
        CW_Update_Shire_State
        CW_Check_Shires_Available_And_Ready
        CW_Get_Physically_Enabled_Shires
*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>
#include <transports/mm_cm_iface/message_types.h>

/* mm specific headers */
#include "config/mm_config.h"
#include "drivers/plic.h"
#include "services/cm_iface.h"
#include "services/log.h"
#include "services/sp_iface.h"
#include "services/sw_timer.h"
#include "workers/cw.h"
#include "services/host_cmd_hdlr.h"

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"
#include "cm_mm_defines.h"

/*! \typedef cw_cb_t
    \brief Compute Worker control block.
    Consists data structures to manage shire state of shires that play
    the role of compute workers
*/
typedef struct cw_cb_t_ {
    uint64_t physically_avail_shires_mask;
    uint64_t booted_shires_mask;
    uint64_t shire_state;
    spinlock_t cm_reset_lock;
    uint32_t timeout_flag;
} cw_cb_t;

/*! \var cw_cb_t CW_CB
    \brief Global Compute Worker Control Block
    \warning Not thread safe!
*/
static cw_cb_t CW_CB __attribute__((aligned(64))) = { 0 };

/*! \def CM_BOOT_MASK_PTR
    \brief Global shared pointer to CM shires boot mask
*/
#define CM_BOOT_MASK_PTR ((uint64_t *)CM_SHIRES_BOOT_MASK_BASEADDR)

/*! \def CW_RESET_CB
    \brief Macro to reset the state of glabals in CW CB
*/
#define CW_RESET_CB(shire_mask)                                        \
    {                                                                  \
        /* Reset state of globals */                                   \
        atomic_and_global_64(CM_BOOT_MASK_PTR, (~shire_mask));         \
        atomic_and_local_64(&CW_CB.booted_shires_mask, (~shire_mask)); \
        atomic_and_local_64(&CW_CB.shire_state, (~shire_mask));        \
        atomic_store_local_32(&CW_CB.timeout_flag, 0);                 \
    }

/************************************************************************
*
*   FUNCTION
*
*       cw_set_init_timeout_cb
*
*   DESCRIPTION
*
*       Sets the timeout flag for CW init and sends IPI
*
*   INPUTS
*
*       thread_id    ID of the thread to send IPI to
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void cw_set_init_timeout_cb(uint8_t thread_id)
{
    atomic_store_local_32(&CW_CB.timeout_flag, 1);

    /* Trigger IPI to respective hart */
    syscall(SYSCALL_IPI_TRIGGER_INT, (1ULL << thread_id), MASTER_SHIRE, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Init
*
*   DESCRIPTION
*
*       Initialize Shires/Minions that serve as compute workers
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t              status success or failure
*
***********************************************************************/
int32_t CW_Init(void)
{
    bool exit_loop = false;
    uint8_t lvdpll_strap = 0;
    int32_t status = STATUS_SUCCESS;
    int32_t sw_timer_idx;
    uint64_t shire_mask = 0;
    uint64_t booted_shires_mask = 0;
    uint64_t sip;

    /* Obtain the number of shires to be used from SP and initialize the CW control block */
    status = SP_Iface_Get_Shire_Mask_And_Strap(&shire_mask, &lvdpll_strap);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Reset the globals and clear the whole shire mask */
    CW_RESET_CB(0xFFFFFFFFFFFFFFFF)

    Log_Write(LOG_LEVEL_DEBUG, "CW_Init:Shire mask from SP: 0x%lx\r\n", shire_mask);

    /* Set the bit for MM shire sync Minions */
    shire_mask = MASK_SET_BIT(shire_mask, MASTER_SHIRE);

    /* Initialize Global CW_CB */
    atomic_store_local_64(&CW_CB.physically_avail_shires_mask, shire_mask);

    /* Initialize the spinlock */
    init_local_spinlock(&CW_CB.cm_reset_lock, 0);

    /* Bring up Compute Workers */
    syscall(SYSCALL_CONFIGURE_COMPUTE_MINION, shire_mask, lvdpll_strap, 0);

    /* Create timeout to wait for all Compute Workers to boot up */
    sw_timer_idx = SW_Timer_Create_Timeout(
        &cw_set_init_timeout_cb, (get_hart_id() & (HARTS_PER_SHIRE - 1)), CW_INIT_TIMEOUT);

    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_WARNING,
            "CW: Unable to register CW init timeout! It may not recover in case of hang\r\n");
    }

    /* Keep polling until all the compute shires have booted up or the timer expires */
    /* TODO: The CMs are sending IPIs once booted. Those seem to be missed by MM.
    Polling would fix the issue for now. */
    while (!exit_loop)
    {
        /* Read pending interrupts (if any) */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        /* The pending external interrupts need to be processed here so that SW timer
        expires based on tick interval. This function is meant to be called from dispatcher
        context. Dispatcher is responsible for processing all the interrupts */
        if (sip & (1 << SUPERVISOR_EXTERNAL_INTERRUPT))
        {
            PLIC_Dispatch();

            if (SW_Timer_Interrupt_Status())
            {
                SW_Timer_Processing();
            }

            /* Check for SW timer timeout */
            if (atomic_compare_and_exchange_local_32(&CW_CB.timeout_flag, 1, 0) == 1)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "CW:Init:Timed-out waiting CW Ack: Pending Shire Mask: 0x%lx\r\n",
                    (~atomic_load_local_64(&CW_CB.booted_shires_mask) & shire_mask));
                status = CW_ERROR_INIT_TIMEOUT;
                exit_loop = true;
            }
        }

        if (status == STATUS_SUCCESS)
        {
            /* Load the global CM shires boot mask */
            booted_shires_mask = atomic_load_global_64(CM_BOOT_MASK_PTR);

            /* Store the global shared shires mask locally */
            atomic_store_local_64(&CW_CB.booted_shires_mask, booted_shires_mask);

            /* Break loop if all compute minions are booted and ready */
            if ((booted_shires_mask & shire_mask) == shire_mask)
            {
                exit_loop = true;
            }
        }
    }

    if (sw_timer_idx >= 0)
    {
        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Process_CM_SMode_Messages
*
*   DESCRIPTION
*
*       Helper to process messages from compute minion firmware running
*       in S mode. Specifically handles exceptions that arise in during
*       compute minion firmware in S mode.
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
void CW_Process_CM_SMode_Messages(void)
{
    int32_t status;

    /* Processes messages from CM from CM > MM unicast circbuff */
    while (1)
    {
        cm_iface_message_t message;

        /* Receive msg from unicast buffer of dispatcher */
        status = CM_Iface_Unicast_Receive(CM_MM_MASTER_HART_UNICAST_BUFF_IDX, &message);

        if (status != STATUS_SUCCESS)
        {
            /* Check for error conditions */
            if ((status != CIRCBUFF_ERROR_BAD_LENGTH) && (status != CIRCBUFF_ERROR_EMPTY))
            {
                Log_Write(LOG_LEVEL_ERROR, "CW:ERROR:CM_To_MM Receive failed. Status code: %d\r\n",
                    status);
            }

            /* No more pending messages left */
            Log_Write(LOG_LEVEL_DEBUG, "CW:CM_To_MM: No pending msg\r\n");
            break;
        }

        switch (message.header.id)
        {
            case CM_TO_MM_MESSAGE_ID_NONE:
                Log_Write(LOG_LEVEL_DEBUG, "CW:CM_TO_MM:MESSAGE_ID_NONE\r\n");
                break;

            case CM_TO_MM_MESSAGE_ID_FW_EXCEPTION:
            {
                /* Update CM State to exception if it was not already in bad condition. */
                CM_Iface_Update_CM_State(CM_STATE_NORMAL, CM_STATE_EXCEPTION);
                const cm_to_mm_message_exception_t *exception =
                    (cm_to_mm_message_exception_t *)&message;

                Log_Write(LOG_LEVEL_CRITICAL, "CW:CM_TO_MM:MESSAGE_ID_FW_EXCEPTION from H%ld\r\n",
                    exception->hart_id);

                /* Report SP of CM FW exception */
                SP_Iface_Report_Error(
                    MM_RECOVERABLE_FW_CM_RUNTIME_ERROR, MM_CM_RUNTIME_EXCEPTION_ERROR);

                /* Send Async error event to Host runtime. */
                status = Device_Async_Error_Event_Handler(
                    DEV_OPS_API_ERROR_TYPE_CM_SMODE_RT_EXCEPTION, (uint32_t)exception->hart_id);

                if (status != STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "CW: Failed in Device Async Error handling (status: %d)\r\n", status);
                }

                break;
            }
            case CM_TO_MM_MESSAGE_ID_FW_ERROR:
            {
                const cm_to_mm_message_fw_error_t *error = (cm_to_mm_message_fw_error_t *)&message;

                /* Report SP of CM FW Error */
                SP_Iface_Report_Error(MM_RECOVERABLE_FW_CM_RUNTIME_ERROR, MM_CM_RUNTIME_FW_ERROR);

                Log_Write(LOG_LEVEL_CRITICAL,
                    "CW:CM_TO_MM:MESSAGE_ID_FW_ERROR from H%ld: Error_code: %d\r\n", error->hart_id,
                    error->error_code);

                break;
            }
            default:
                /* Report SP of CM FW Error */
                SP_Iface_Report_Error(
                    MM_RECOVERABLE_FW_CM_RUNTIME_ERROR, MM_CM2MM_UNKOWN_MESSAGE_ERROR);

                Log_Write(LOG_LEVEL_ERROR, "CW:CM_TO_MM:Unknown message id = 0x%x\r\n",
                    message.header.id);
                break;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Get_Physically_Enabled_Shires
*
*   DESCRIPTION
*
*       Get the actual physically available shires
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    Get available shires
*
***********************************************************************/
uint64_t CW_Get_Physically_Enabled_Shires(void)
{
    return atomic_load_local_64(&CW_CB.physically_avail_shires_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Get_Booted_Shires
*
*   DESCRIPTION
*
*       Get the booted available shires
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    Returns booted shires mask
*
***********************************************************************/
uint64_t CW_Get_Booted_Shires(void)
{
    return atomic_load_local_64(&CW_CB.booted_shires_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Update_Shire_State
*
*   DESCRIPTION
*
*       Set the state of a group of shires
*
*   INPUTS
*
*       shire_mask    Shire group mask
*       shire_state   Shire state
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CW_Update_Shire_State(uint64_t shire_mask, cw_shire_state_t shire_state)
{
    if (shire_state == CW_SHIRE_STATE_FREE)
    {
        atomic_and_local_64(&CW_CB.shire_state, ~shire_mask);
    }
    else
    {
        atomic_or_local_64(&CW_CB.shire_state, shire_mask);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       CW_Check_Shires_Available_And_Free
*
*   DESCRIPTION
*
*       Check all shires specified by mask are available and ready.
*
*   INPUTS
*
*       uint64_t   Shire mask to check
*
*   OUTPUTS
*
*       int32_t    Success status or error code
*
***********************************************************************/
int32_t CW_Check_Shires_Available_And_Free(uint64_t shire_mask)
{
    int32_t status = CW_SHIRES_NOT_FREE;

    /* Verify if all the given shires are booted */
    if ((atomic_load_local_64(&CW_CB.booted_shires_mask) & shire_mask) == shire_mask)
    {
        /* Shire state definition is 0 if free else 1 if busy */
        /* Hence flips status to compare with Requested Shire Mask */
        if ((~atomic_load_local_64(&CW_CB.shire_state) & shire_mask) == shire_mask)
        {
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        status = CW_SHIRE_UNAVAILABLE;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       CW_CM_Configure_And_Wait_For_Boot
*
*   DESCRIPTION
*
*       Reset booted shire mask and wait after performing CM warm reset
*
*   INPUTS
*
*       uint64_t   Shire mask to check
*
*   OUTPUTS
*
*       int32_t    Success status or error code
*
***********************************************************************/
int32_t CW_CM_Configure_And_Wait_For_Boot(uint64_t shire_mask)
{
    bool exit_loop = false;
    int32_t status = STATUS_SUCCESS;
    int32_t sw_timer_idx;
    uint64_t booted_shires_mask;

    /*TODO: Do not reset MM Shire until we have support to reset one neigh in MM shire .*/
    shire_mask = shire_mask & (~MM_SHIRE_MASK);

    /* Acquire the lock */
    acquire_local_spinlock(&CW_CB.cm_reset_lock);

    CW_RESET_CB(shire_mask)

    /* Put all Compute Minion Neigh Logic into reset */
    syscall(SYSCALL_DISABLE_NEIGH, shire_mask, 0, 0);

    /* Re-init MM-CM Control Blocks */
    CM_Iface_Init();

    /* Bring all Compute Minion Neigh Logic out of reset */
    syscall(SYSCALL_ENABLE_NEIGH, shire_mask, 0, 0);

    /* Create timeout to wait for all Compute Workers to boot up */
    sw_timer_idx = SW_Timer_Create_Timeout(
        &cw_set_init_timeout_cb, (get_hart_id() & (HARTS_PER_SHIRE - 1)), CW_INIT_TIMEOUT);

    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_WARNING,
            "CW: Unable to register CW init timeout! It may not recover in case of hang\r\n");
    }

    /* Wait for all workers to be initialized */
    do
    {
        /* Check for SW timer timeout */
        if (atomic_compare_and_exchange_local_32(&CW_CB.timeout_flag, 1, 0) == 1)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "CW:Reset:Timed-out waiting CW Ack: Pending Shire Mask: 0x%lx\r\n",
                (~atomic_load_local_64(&CW_CB.booted_shires_mask) & shire_mask));
            status = CW_ERROR_INIT_TIMEOUT;
            exit_loop = true;
        }

        if (status == STATUS_SUCCESS)
        {
            /* Load the global CM shires boot mask */
            booted_shires_mask = atomic_load_global_64(CM_BOOT_MASK_PTR);

            /* Store the global shared shires mask locally */
            atomic_store_local_64(&CW_CB.booted_shires_mask, booted_shires_mask);

            /* Break loop if all compute minions are booted and ready */
            if ((booted_shires_mask & shire_mask) == shire_mask)
            {
                Log_Write(LOG_LEVEL_CRITICAL,
                    "CW: All CW re-booted successfully. Shire Mask:0x%lx!\r\n", shire_mask);
                exit_loop = true;
            }
        }
    } while (!exit_loop);

    if (sw_timer_idx >= 0)
    {
        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
    }

    /* Release the lock */
    release_local_spinlock(&CW_CB.cm_reset_lock);

    return status;
}
