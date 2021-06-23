/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------

*************************************************************************/
/*! \file bl2_watchdog.c
    \brief A C module that implements the WDT functions

    Public interfaces:
        watchdog_init
        watchdog_error_init
        watchdog_start
        watchdog_stop
        watchdog_kick
        get_watchdog_timeout
        get_watchdog_max_timeout
*/
/***********************************************************************/
#include <stdio.h>
#include "log.h"
#include "io.h"
#include <bl2_watchdog.h>
#include "hal_device.h"
#include <spio_wdt.h>
#include <interrupt.h>
#include <bl2_pmic_controller.h>
#include "dm_event_control.h"
#include "bl2_exception.h"

/* The driver can populate this structure with the defaults that will be used during the init
 phase.*/

static struct watchdog_control_block wdog_control_block __attribute__((section(".data")));

/************************************************************************
*
*   FUNCTION
*
*       watchdog_init
*
*   DESCRIPTION
*
*       This function initializes WDT
*
*   INPUTS
*
*       timeout_msec    Timeout in miliseconds
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t watchdog_init(uint32_t timeout_msec)
{
    /*
      Init Watchdog counter with the given timeout 
      Enable interrupt handler for the SPIO_WDT_INTR
      Enable PMIC interface to handle second interrupt
      and reset the system, spio_wdt_sys_rstn*.
      Start the watch dog 
    */

    wdog_control_block.timeout_msec = timeout_msec;
    
    //TODO: calculate precise register value for milisec provided
    iowrite32(R_SP_WDT_BASEADDR + SPIO_DW_APB_WDT_WDT_TORR_ADDRESS, timeout_msec);

    INT_enableInterrupt(SPIO_PLIC_WDT_INTR, 1, watchdog_isr);

    watchdog_start();

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       watchdog_error_init
*
*   DESCRIPTION
*
*       This function initializes error callback
*
*   INPUTS
*
*       event_cb    Callback function
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t watchdog_error_init(dm_event_isr_callback event_cb)
{
    /* Save the handle to the timeout callback */
    wdog_control_block.event_cb = event_cb;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       watchdog_start
*
*   DESCRIPTION
*
*       This function start WDT
*
*   INPUTS
*
*       none    
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t watchdog_start(void)
{
    uint32_t cr_reg = 0;

    cr_reg = (uint32_t)SPIO_DW_APB_WDT_WDT_CR_RMOD_MODIFY(cr_reg, 1);
    cr_reg = (uint32_t)SPIO_DW_APB_WDT_WDT_CR_WDT_EN_MODIFY(cr_reg, 1);
    iowrite32(R_SP_WDT_BASEADDR + SPIO_DW_APB_WDT_WDT_CR_ADDRESS, cr_reg);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       watchdog_stop
*
*   DESCRIPTION
*
*       This function stops WDT
*
*   INPUTS
*
*       none    
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t watchdog_stop(void)
{
    iowrite32(R_SP_WDT_BASEADDR + SPIO_DW_APB_WDT_WDT_CR_ADDRESS, SPIO_DW_APB_WDT_WDT_CR_WDT_EN_WDT_EN_DISABLED);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       watchdog_kick
*
*   DESCRIPTION
*
*       This function restarts WDT
*
*   INPUTS
*
*       none    
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
void watchdog_kick(void)
{
    //Feed the wdog
    iowrite32(R_SP_WDT_BASEADDR + SPIO_DW_APB_WDT_WDT_CRR_ADDRESS, SPIO_DW_APB_WDT_WDT_CRR_WDT_CRR_WDT_CRR_RESTART);
}

/************************************************************************
*
*   FUNCTION
*
*       get_watchdog_timeout
*
*   DESCRIPTION
*
*       This function reads WDT timeout
*
*   INPUTS
*
*       pointer to timeout variable to be populated    
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t get_watchdog_timeout(uint32_t *timeout_msec)
{
    *timeout_msec = wdog_control_block.timeout_msec;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_watchdog_max_timeout
*
*   DESCRIPTION
*
*       This function reads WDT max timeout
*
*   INPUTS
*
*       pointer to timeout variable to be populated    
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
int32_t get_watchdog_max_timeout(uint32_t *timeout_msec)
{
    *timeout_msec = wdog_control_block.max_timeout_msec;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       watchdog_isr
*
*   DESCRIPTION
*
*       WDT ISR
*
*   INPUTS
*
*       none   
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
void watchdog_isr(void)
{
    /* Restart and clear the interrupt */
    iowrite32(R_SP_WDT_BASEADDR + SPIO_DW_APB_WDT_WDT_CRR_ADDRESS, SPIO_DW_APB_WDT_WDT_CRR_WDT_CRR_WDT_CRR_RESTART);

    bl2_dump_stack_frame();

    /* Invoke the event handler callback */
    if (wdog_control_block.event_cb)
    {
        struct event_message_t message;
        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, WDOG_INTERNAL_TIMEOUT,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, 0, 0);
        wdog_control_block.event_cb(CORRECTABLE, &message);
    }
}
