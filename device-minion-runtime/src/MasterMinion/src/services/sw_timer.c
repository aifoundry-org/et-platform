/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* argeement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file sw_timer.c
    \brief A C module that implements the software timer to register the
    timeouts for commands while launching. SW_TIMER uses a PU Timer
    chanel underneath, which triggers a callback periodically. At each
    triger, the callback will check if some commands is to abort due to
    timeout and will call the abort callback for the specific command.

    Public interfaces:
        SW_Timer_Init
        SW_Timer_Create_Timeout
        SW_Timer_Cancel_Timeout
        SW_Timer_Get_Elapsed_Time
        SW_Timer_Processing
*/
/***********************************************************************/
#include "services/log.h"
#include "services/sw_timer.h"
#include "drivers/pu_timers.h"
#include "sync.h"

/*! \struct cmd_timeout_instance_
    \brief Holds the information to register a timeout
*/
typedef struct cmd_timeout_instance_ {
    uint64_t expiration_time; /* Time at/after which timeout triggers */
    void     (*timeout_callback_fn)(uint8_t);
    uint8_t  callback_arg;
} cmd_timeout_t;

/*! \struct sw_timer_cb_
    \brief SW Timer Control Block structure. Used to maintain the list
    of timeouts, PU Timer channel being used and global time elasped
    till now
*/
typedef struct sw_timer_cb_ {
    uint64_t     accum_period;
    spinlock_t   resource_lock;
    cmd_timeout_t cmd_timeout_cb[SW_TIMER_MAX_SLOTS];
} sw_timer_cb_t;

/*! \var sw_timer_cb_t SW_TIMER_CB
    \brief Global SW Timer Control Block
    \warning Not thread safe!
*/
static sw_timer_cb_t SW_TIMER_CB __attribute__((aligned(64))) = {0};

/*! \var bool SW_Timer_Interrupt_Flag
    \brief Global Flag to indicate PU Timer interrupt triggered
    \warning Not thread safe!
*/
static volatile bool SW_Timer_Interrupt_Flag __attribute__((aligned(64))) = false;

/*! \fn static inline int8_t get_free_slot(void)
    \brief Returns the index of free timeout slot
    \return Index of free slot
*/
static inline int8_t get_free_slot(void)
{
    for(int8_t sw_timer_idx = 0; sw_timer_idx < SW_TIMER_MAX_SLOTS; sw_timer_idx++)
    {
        if(atomic_load_local_64(
            &SW_TIMER_CB.cmd_timeout_cb[sw_timer_idx].expiration_time) ==
            SW_TIME_FREE_SLOT_FLAG)
            {
                return sw_timer_idx;
            }
    }

    return -1;
}

/*! \fn bool SW_Timer_Interrupt_Status(void)
    \brief Get the status of the SW Timer Flag
    \returns status of the SW Timer Int Flag
*/
_Bool SW_Timer_Interrupt_Status(void)
{
    return SW_Timer_Interrupt_Flag;
}

/************************************************************************
*
*   FUNCTION
*
*      SW_Timer_Processing
*
*   DESCRIPTION
*
*       Function to handle periodic PU Timer triggers
*       and check the timeout for added timers so far.
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
void SW_Timer_Processing(void)
{
    uint64_t accum_period;
    void (*timeout_callback_fn)(uint8_t);

    if(SW_Timer_Interrupt_Flag)
    {
        SW_Timer_Interrupt_Flag = false;
        asm volatile("fence");
    }

    accum_period = atomic_add_local_64(&SW_TIMER_CB.accum_period, SW_TIMER_HW_COUNT_PER_SEC);
    accum_period += SW_TIMER_HW_COUNT_PER_SEC;

    for (uint8_t i = 0; i < SW_TIMER_MAX_SLOTS; i++)
    {
        if(atomic_load_local_64(&SW_TIMER_CB.cmd_timeout_cb[i].expiration_time)
            <= accum_period)
        {
            timeout_callback_fn = (void*)(uint64_t)atomic_load_local_64(
                (void*)&SW_TIMER_CB.cmd_timeout_cb[i].timeout_callback_fn);
            timeout_callback_fn(atomic_load_local_8(
                &SW_TIMER_CB.cmd_timeout_cb[i].callback_arg));

            /* Use max uint64_t value as falg to mark this slot of time as free */
            atomic_store_local_64(&SW_TIMER_CB.cmd_timeout_cb[i].expiration_time,
                SW_TIME_FREE_SLOT_FLAG);
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SW_Timer_isr
*
*   DESCRIPTION
*
*       Interrupt handler PU Timers expiration
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
static void SW_Timer_isr(void)
{
    Log_Write(LOG_LEVEL_DEBUG,
        "Dispatcher:PU Timer Expiration interrupt!\r\n");

    SW_Timer_Interrupt_Flag = true;

    PU_Timer_Interrupt_Clear();
}

/************************************************************************
*
*   FUNCTION
*
*       SW_Timer_Init
*
*   DESCRIPTION
*
*       Initializes the SW Timer
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       SW_TIMER_STATUS_e         status success or error
*
***********************************************************************/
SW_TIMER_STATUS_e SW_Timer_Init(void)
{
    for(uint8_t i = 0; i < SW_TIMER_MAX_SLOTS; i++)
    {
        /* Use max uint64_t value as flag to mark this slot of time as free */
        atomic_store_local_64(&SW_TIMER_CB.cmd_timeout_cb[i].expiration_time,
            SW_TIME_FREE_SLOT_FLAG);
    }

    /* Init the HW timer */
    PU_Timer_Init(SW_Timer_isr, SW_TIMER_HW_COUNT_PER_SEC);

    return SW_TIMER_OPERATION_SUCCESS;
}


/************************************************************************
*
*   FUNCTION
*
*       SW_Timer_Create_Timeout
*
*   DESCRIPTION
*
*       Add a a timer for new incomming command
*
*   INPUTS
*
*       timeout_callback_fn   Callback to trigger at timeout
*       arg                   Argument for callback
*       sw_ticks              SW ticks for timeout the cammand
*
*   OUTPUTS
*
*       free_timer_slot       SWTimer slot used to create timeout
*
***********************************************************************/
int8_t SW_Timer_Create_Timeout(void (*timeout_callback_fn)(uint8_t),
                                uint8_t callback_arg, uint32_t sw_ticks)
{
    int8_t free_timer_slot;

    /* Acquire the lock */
    acquire_local_spinlock(&SW_TIMER_CB.resource_lock);

    free_timer_slot = get_free_slot();
    if(free_timer_slot >= 0)
    {
        atomic_store_local_64(
            (void*)&SW_TIMER_CB.cmd_timeout_cb[free_timer_slot].timeout_callback_fn,
            (uint64_t)timeout_callback_fn);
        atomic_store_local_8(&SW_TIMER_CB.cmd_timeout_cb[free_timer_slot].callback_arg,
            callback_arg);
        /* Store expiration_time = sw_ticks + accum_period + elapsed_time */
        atomic_store_local_64(
            &SW_TIMER_CB.cmd_timeout_cb[free_timer_slot].expiration_time,
            SW_TIMER_SW_TICKS_TO_HW_COUNT(sw_ticks)
            + atomic_load_local_64(&SW_TIMER_CB.accum_period)
            + SW_Timer_Get_Elapsed_Time());
    }

    /* Release the lock */
    release_local_spinlock(&SW_TIMER_CB.resource_lock);

    return free_timer_slot;
}

/************************************************************************
*
*   FUNCTION
*
*       SW_Timer_Cancel_Timeout
*
*   DESCRIPTION
*
*       Cancel a timer for a command
*
*   INPUTS
*
*       sw_timer_idx     Slot number of the timer to cancel
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SW_Timer_Cancel_Timeout(uint8_t sw_timer_idx)
{
    /* Use max uint64_t value as flag to mark this slot of timer as free */
    atomic_store_local_64(
        &SW_TIMER_CB.cmd_timeout_cb[sw_timer_idx].expiration_time,
        SW_TIME_FREE_SLOT_FLAG);

    /* Clear the callback */
    atomic_store_local_64((void*)&SW_TIMER_CB.cmd_timeout_cb[sw_timer_idx].timeout_callback_fn, 0U);
}

/************************************************************************
*
*   FUNCTION
*
*       SW_Timer_Get_Elapsed_Time
*
*   DESCRIPTION
*
*       Returns the elapsed time from last Periodic timer update
*       
*   INPUTS
*
*       sw_timer_idx     Slot number of the timer
*
*   OUTPUTS
*
*       uint32_t         Elapsed delta time
*
***********************************************************************/
uint32_t SW_Timer_Get_Elapsed_Time(void)
{
    return (SW_TIMER_HW_COUNT_PER_SEC - PU_Timer_Get_Current_Value());
}
