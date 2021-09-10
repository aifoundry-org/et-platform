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
#include "portmacro.h"

#include <et-trace/encoder.h>

/* Local functions */
static void dump_stack_frame_and_csrs(const void *stack_frame,
    struct dev_context_registers_t *context);
static void *dump_perf_globals_trace(void *buf);
static void dump_power_globals_trace(void *buf);

/*! \def DUMP_THROTTLE_RESIDENCY(THROTTLE_STATE)
    \brief Dumps throttle residency to memory
*/
#define DUMP_THROTTLE_RESIDENCY(THROTTLE_STATE)                    \
    if (0 != get_throttle_residency(THROTTLE_STATE, &residency))   \
    {                                                              \
        memcpy(trace_buf, &residency, sizeof(struct residency_t)); \
        trace_buf += sizeof(uint64_t);                             \
    }

/*! \def DUMP_POWER_RESIDENCY(POWER_STATE)
    \brief Dumps power residency to memory
*/
#define DUMP_POWER_RESIDENCY(POWER_STATE)                          \
    if (0 != get_power_residency(POWER_STATE, &residency))         \
    {                                                              \
        memcpy(trace_buf, &residency, sizeof(struct residency_t)); \
        trace_buf += sizeof(uint64_t);                             \
    }

/************************************************************************
*
*   FUNCTION
*
*       bl2_exception_entry
*
*   DESCRIPTION
*
*       High level exception handler - dumps the system state to trace
*       buffer or console in case of exceptions.
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
__attribute__((noreturn)) void bl2_exception_entry(const void *stack_frame)
{
    struct dev_context_registers_t context = {0};
    struct trace_control_block_t *trace_cb = Trace_Get_SP_CB();
    uint8_t *trace_buf;

    /* Disable global interrupts - no nested exceptions */
    portDISABLE_INTERRUPTS();

    /* Dump the arch and chip specific registers. No chip registers are
    currently saved in the context switch code. */
    dump_stack_frame_and_csrs(stack_frame, &context);

    /* Log the execution stack event to trace */
    trace_buf = Trace_Execution_Stack(trace_cb, &context);

    /* Generate exception event for host */
    SP_Exception_Event(SP_TRACE_GET_ENTRY_OFFSET(trace_buf, trace_cb));

    /* TODO: Need to dump global using Trace_Custom_Event() */
    (void)dump_perf_globals_trace;
    (void)dump_power_globals_trace;

    Log_Write(LOG_LEVEL_CRITICAL, "SP Spining in Exception handler..\r\n");

    /* No recovery for now - spin forever */
    while (1)
        ;
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
*       Pointer to region in trace buffer reserved for exception stack entry
*
***********************************************************************/
uint8_t *bl2_dump_stack_frame(void)
{
    struct dev_context_registers_t context = {0};
    uint64_t stack_frame;
    uint8_t *trace_buf;

    /* For dumping the stack frame outside of exception context such as
        wdog interrupt, the stack is assumed to be present in the a7 register.
        The trap handler saves the sp in the a7 and it is preserved till we reach
        the driver ISR handler.
    */
    asm volatile("mv %0, a7" : "=r"(stack_frame));

    /* Dump the context */
    dump_stack_frame_and_csrs((void*)stack_frame, &context);

    /* Log the execution stack event to trace */
    trace_buf = Trace_Execution_Stack(Trace_Get_SP_CB(), &context);

    return trace_buf;
}

/************************************************************************
*
*   FUNCTION
*
*       dump_stack_frame_and_csrs
*
*   DESCRIPTION
*
*       This function dumps the stack frame state and CSRs to trace buffer
*       or console.
*
*   INPUTS
*
*       stack_frame    Pointer to stack frame
*       context        Pointer to context registers structure
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dump_stack_frame_and_csrs(const void *stack_frame,
    struct dev_context_registers_t *context)
{
    const uint64_t *stack_pointer = (const uint64_t *)stack_frame;

    /* Dump the stack frame - for stack frame defintion,
        see the comments in the portASM.c file */

    /* Move the stack pointer to x1 saved location */
    stack_pointer++;

    /* Save x1 first */
    context->gpr[0] = *stack_pointer;

    /* Move the stack pointer to x5 saved location */
    stack_pointer++;

    /* Dump x5 to x31 to specified context structure */
    memcpy((void*)&context->gpr[4], stack_pointer,
        SP_EXCEPTION_STACK_FRAME_SIZE - (sizeof(uint64_t) * 1));

    /* Dump CSRs to the specified context structure */
    asm volatile("csrr %0, mcause\n"
                 "csrr %1, mstatus\n"
                 "csrr %2, mepc\n"
                 "csrr %3, mtval"
                 : "=r"(context->cause), "=r"(context->status),
                   "=r"(context->epc), "=r"(context->tval));
}

/************************************************************************
*
*   FUNCTION
*
*       dump_perf_globals_trace
*
*   DESCRIPTION
*
*       Dumps important performance globals to trace buffer
*
*   INPUTS
*
*       trace buffer
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
static void *dump_perf_globals_trace(void *buf)
{
    uint64_t *trace_buf = (uint64_t *)buf;
    struct asic_frequencies_t asic_frequencies;
    struct dram_bw_t dram_bw;
    struct max_dram_bw_t max_dram_bw;
    uint32_t pct_cap = 0;
    uint64_t last_ts = 0;

    if (0 != get_module_asic_frequencies(&asic_frequencies))
    {
        memcpy(trace_buf, &asic_frequencies, sizeof(struct asic_frequencies_t));
        trace_buf += sizeof(struct asic_frequencies_t);
    }
    if (0 != get_module_dram_bw(&dram_bw))
    {
        memcpy(trace_buf, &dram_bw, sizeof(struct dram_bw_t));
        trace_buf += sizeof(struct dram_bw_t);
    }
    if (0 != get_module_max_dram_bw(&max_dram_bw))
    {
        memcpy(trace_buf, &max_dram_bw, sizeof(struct max_dram_bw_t));
        trace_buf += sizeof(struct max_dram_bw_t);
    }
    if (0 != get_dram_capacity_percent(&pct_cap))
    {
        *trace_buf = pct_cap;
        trace_buf += sizeof(pct_cap);
    }
    if (0 != get_last_update_ts(&last_ts))
    {
        *trace_buf++ = last_ts;
    }

    return (void *)trace_buf;
}

/************************************************************************
*
*   FUNCTION
*
*       dump_power_globals_trace
*
*   DESCRIPTION
*
*       Dumps important power globals to trace buffer
*
*   INPUTS
*
*       trace buffer
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
static void dump_power_globals_trace(void *buf)
{
    uint8_t *trace_buf = (uint8_t *)buf;
    struct module_uptime_t module_uptime = { 0 };
    struct module_voltage_t module_voltage = { 0 };
    power_state_e power_state = 0;
    uint8_t tdp_level = 0;
    struct residency_t residency = { 0, 0, 0, 0 };
    uint8_t temp = 0;

    if (0 != get_module_power_state(&power_state))
    {
        *trace_buf++ = power_state;
    }
    if (0 != get_module_tdp_level(&tdp_level))
    {
        *trace_buf++ = tdp_level;
    }
    if (0 != get_module_current_temperature(&temp))
    {
        *trace_buf++ = temp;
    }

    if (0 != get_module_soc_power(&temp))
    {
        *trace_buf++ = temp;
    }

    if (0 != get_soc_max_temperature(&temp))
    {
        *trace_buf++ = temp;
    }

    if (0 != get_module_uptime(&module_uptime))
    {
        memcpy(trace_buf, &module_uptime, sizeof(struct module_uptime_t));
        trace_buf += sizeof(struct module_uptime_t);
    }

    if (0 != get_module_voltage(&module_voltage))
    {
        memcpy(trace_buf, &module_voltage, sizeof(struct module_voltage_t));
        trace_buf += sizeof(struct module_voltage_t);
    }

    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_UP)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_DOWN)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_THERMAL_DOWN)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_SAFE)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_THERMAL_SAFE)

    DUMP_POWER_RESIDENCY(POWER_STATE_MAX_POWER)
    DUMP_POWER_RESIDENCY(POWER_STATE_MANAGED_POWER)
    DUMP_POWER_RESIDENCY(POWER_STATE_SAFE_POWER)
    DUMP_POWER_RESIDENCY(POWER_STATE_LOW_POWER)
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Exception_Event
*
*   DESCRIPTION
*
*       Generate exception event to host
*
*   INPUTS
*
*       trace_buffer_offset    Offset in trace buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SP_Exception_Event(uint64_t trace_buffer_offset)
{
    struct event_message_t message;

    /* add details in message header and fill payload */
    FILL_EVENT_HEADER(&message.header, SP_RUNTIME_EXCEPT, sizeof(struct event_message_t))
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, timer_get_ticks_count(), trace_buffer_offset)

    /* Post message to the queue */
    if (0 != SP_Host_Iface_CQ_Push_Cmd((void *)&message, sizeof(message)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SP_Exception_Event: push to CQ failed!\n");
    }
}
