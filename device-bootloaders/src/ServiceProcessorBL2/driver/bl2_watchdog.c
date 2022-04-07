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
#include "etsoc/isa/io.h"
#include <bl2_watchdog.h>
#include <interrupt.h>
#include <bl2_pmic_controller.h>
#include "dm_event_control.h"
#include "bl2_exception.h"

#include "hwinc/hal_device.h"
#include "hwinc/sp_wdt.h"

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
    iowrite32(R_SP_WDT_BASEADDR + WDT_WDT_TORR_ADDRESS, timeout_msec);

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

    cr_reg = (uint32_t)WDT_WDT_CR_RMOD_MODIFY(cr_reg, 1);
    cr_reg = (uint32_t)WDT_WDT_CR_WDT_EN_MODIFY(cr_reg, 1);
    iowrite32(R_SP_WDT_BASEADDR + WDT_WDT_CR_ADDRESS, cr_reg);

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
    iowrite32(R_SP_WDT_BASEADDR + WDT_WDT_CR_ADDRESS, WDT_WDT_CR_WDT_EN_WDT_EN_DISABLED);

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
    iowrite32(R_SP_WDT_BASEADDR + WDT_WDT_CRR_ADDRESS, WDT_WDT_CRR_WDT_CRR_WDT_CRR_RESTART);
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
*       Stack frame is saved to a7 register before invoking WDT ISR, this
*       helps in dumping stack frame and CSRS to trace. Exception trace 
*       buffer is used to dump stack and CSRS information. Trace offset 
*       is then sent to driver to print information on console.
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
    uint64_t stack_frame;
    uint8_t *trace_buf;

    /* Restart and clear the interrupt */
    iowrite32(R_SP_WDT_BASEADDR + WDT_WDT_CRR_ADDRESS, WDT_WDT_CRR_WDT_CRR_WDT_CRR_RESTART);

    /* For dumping the stack frame outside of exception context such as
    wdog interrupt, the stack is assumed to be present in the a7 register.
    The trap handler saves the sp in the a7 and it is preserved till we reach
    the driver ISR handler.
    */
    asm volatile("mv %0, a7" : "=r"(stack_frame));

    /* Dump stack frame to trace buffer in lock-less manner */
    trace_buf = Trace_Exception_Dump_Context((void *)stack_frame);

    /* BL2 report event to post message directly to CQ*/
    BL2_Report_Event(SP_TRACE_GET_ENTRY_OFFSET(trace_buf), SP_RUNTIME_HANG);
}
