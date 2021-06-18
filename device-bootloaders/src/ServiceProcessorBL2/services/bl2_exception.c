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
/*! \file bl2_exception.c
    \brief A C module that implements exception handling

    Public interfaces:
    bl2_exception_entry
    bl2_dump_stack_frame
*/
/***********************************************************************/
#include "bl2_exception.h"

/* Local functions */
static void dump_stack_frame(void *stack_frame);
static void dump_csrs(void);

/************************************************************************
*
*   FUNCTION
*
*       bl2_exception_entry
*
*   DESCRIPTION
*
*       High level exception handler - dumps the system state to trace buffer or console
*       in case of exceptions.

*   INPUTS
*
*       stack_frame    Pointer to stack frame
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void bl2_exception_entry(void *stack_frame)
{
    Log_Write (LOG_LEVEL_CRITICAL, "SP runtime exception handler: %s\n", __func__);

    /* Dump the arch and chip sepcifc registers. No chip registers are currently saved
        in the context switch code.
    */
    dump_stack_frame(stack_frame);

    /* Dump the important CSRs */
    dump_csrs();

    /* Dump the globals for performance service */
    dump_perf_globals();

    /* Dump the globals for power and thermal service */
    dump_power_globals();

    /* No recovery for now - spin forever */
    while(1);
}

/************************************************************************
*
*   FUNCTION
*
*       bl2_dump_stack_frame
*
*   DESCRIPTION
*
*       This function  dumps the stack frame for non-exception traps such as
*       watch dog interrupts.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void bl2_dump_stack_frame(void)
{
    uint64_t stack_frame;

    /* For dumping the stack frame outside of exception context such as
        wdog interrupt, the stack is assumed to be present in the a7 register.
        The trap handler saves the sp in the a7 and it is preserved till we reach
        the driver ISR handler.
    */
    asm volatile ("mv %0, a7": "=r"(stack_frame));
    dump_stack_frame((void *)stack_frame);
    dump_csrs();
}

/************************************************************************
*
*   FUNCTION
*
*       bl2_exception_entry
*
*   DESCRIPTION
*
*       This function dumps the stack frame state to trace buffer or console.
*
*   INPUTS
*
*       stack_frame    Pointer to stack frame
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dump_stack_frame(void *stack_frame)
{
    uint64_t *stack_pointer = (uint64_t *)stack_frame;
    uint64_t idx;

    /* Dump the stack frame - for stack frame defintion,
        see the comments in the portASM.c file */

    /* Move the stack pointer to x1 saved location */
    stack_pointer++;

    /* Log the x1 value */
    Log_Write(LOG_LEVEL_CRITICAL, "x1 = 0x%lx\n", *stack_pointer++);

    /* Log x5-x31,x2-x4 are not preserved */
    for (idx = 5; idx < 32; idx++)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "x%lu = 0x%lx\n", idx, *stack_pointer++);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dump_csrs
*
*   DESCRIPTION
*
*       This function dumps CSRs to trace-buffer/console
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
static void dump_csrs(void)
{
    uint64_t mcause_reg, mstatus_reg, mepc_reg, mtval_reg;

    asm volatile ("csrr %0, mcause\n"
                "csrr %1, mstatus\n"
                "csrr %2, mepc\n"
                "csrr %3, mtval"
                :"=r"(mcause_reg),"=r"(mstatus_reg),"=r"(mepc_reg), "=r"(mtval_reg));

    Log_Write(LOG_LEVEL_CRITICAL,
                "mcause = 0x%lx\n mstatus = 0x%lx\n mepc = 0x%lx\n mtval = 0x%lx\n",
                mcause_reg, mstatus_reg, mepc_reg, mtval_reg);

    return;
}