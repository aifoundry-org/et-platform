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

#define ET_TRACE_ENCODER_IMPL
#include <et_trace.h>

/* Local functions */
static void dump_stack_frame(const void *stack_frame, void *trace_buf);
static void dump_csrs(void *buf);
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
*       SP_Exception_Trace_Buffer_Reserve
*
*   DESCRIPTION
*
*       This function reserves region for SP exception in SP trace buffer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint8_t*    Pointer to region in trace buffer reserved for
*                   exception entry.
*
***********************************************************************/
uint8_t *SP_Exception_Trace_Buffer_Reserve(void)
{
    uint64_t exception_entry_size =
        sizeof(struct trace_entry_header_t) + SP_EXCEPTION_FRAME_SIZE + SP_GLOBALS_SIZE;
    uint8_t *exception_entry_ptr = Trace_Buffer_Reserve(Trace_Get_SP_CB(), exception_entry_size);
    struct trace_entry_header_t *entry_header = (struct trace_entry_header_t *)exception_entry_ptr;
    entry_header->generic.cycle = timer_get_ticks_count();
    entry_header->generic.type = TRACE_TYPE_EXCEPTION;
    return exception_entry_ptr;
}

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
__attribute__((noreturn)) void bl2_exception_entry(const void *stack_frame)
{
    /* Disable global interrupts - no nested exceptions */
    portDISABLE_INTERRUPTS();

    uint8_t *trace_buf = SP_Exception_Trace_Buffer_Reserve();
    uint8_t *trace_data_ptr = trace_buf + sizeof(struct trace_entry_header_t);

    /* Dump the arch and chip sepcifc registers. No chip registers are currently saved
        in the context switch code.
    */
    dump_stack_frame(stack_frame, (void *)trace_data_ptr);

    trace_data_ptr += SP_EXCEPTION_STACK_FRAME_SIZE;

    /* Dump the important CSRs */
    dump_csrs((void *)trace_data_ptr);

    trace_data_ptr += SP_EXCEPTION_CSRS_FRAME_SIZE;

    /* Dump the globals for performance service */
    trace_data_ptr = dump_perf_globals_trace((void *)trace_data_ptr);

    /* Dump the globals for power and thermal service */
    dump_power_globals_trace((void *)trace_data_ptr);

    SP_Exception_Event(SP_TRACE_GET_ENTRY_OFFSET(trace_buf, Trace_Get_SP_CB()));

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
    uint64_t stack_frame;
    uint8_t *trace_buf = SP_Exception_Trace_Buffer_Reserve();
    uint8_t *trace_data_ptr = trace_buf + sizeof(struct trace_entry_header_t);

    /* For dumping the stack frame outside of exception context such as
        wdog interrupt, the stack is assumed to be present in the a7 register.
        The trap handler saves the sp in the a7 and it is preserved till we reach
        the driver ISR handler.
    */
    asm volatile("mv %0, a7" : "=r"(stack_frame));
    dump_stack_frame((void *)stack_frame, (void *)trace_data_ptr);
    trace_data_ptr += SP_EXCEPTION_STACK_FRAME_SIZE;
    dump_csrs((void *)trace_data_ptr);

    return trace_buf;
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
static void dump_stack_frame(const void *stack_frame, void *buf)
{
    uint64_t *trace_buf = (uint64_t *)buf;
    const uint64_t *stack_pointer = (const uint64_t *)stack_frame;

    /* Dump the stack frame - for stack frame defintion,
        see the comments in the portASM.c file */

    /* Move the stack pointer to x1 saved location */
    stack_pointer++;

    /* dum stack frame on trace buffer */
    memcpy(trace_buf, stack_pointer, SP_EXCEPTION_STACK_FRAME_SIZE);
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
static void dump_csrs(void *buf)
{
    uint64_t *trace_buf = (uint64_t *)buf;
    uint64_t mcause_reg;
    uint64_t mstatus_reg;
    uint64_t mepc_reg;
    uint64_t mtval_reg;

    asm volatile("csrr %0, mcause\n"
                 "csrr %1, mstatus\n"
                 "csrr %2, mepc\n"
                 "csrr %3, mtval"
                 : "=r"(mcause_reg), "=r"(mstatus_reg), "=r"(mepc_reg), "=r"(mtval_reg));

    /* log above reterived registers to trace */
    *trace_buf++ = mepc_reg;
    *trace_buf++ = mstatus_reg;
    *trace_buf++ = mtval_reg;
    *trace_buf++ = mcause_reg;

    return;
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
*       buf                   trace buffer
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
void SP_Exception_Event(uint64_t buf)
{
    struct event_message_t message;

    /* add details in message header and fill payload */
    FILL_EVENT_HEADER(&message.header, SP_RUNTIME_EXCEPT, sizeof(struct event_message_t))
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, timer_get_ticks_count(), buf)

    /* Post message to the queue */
    if (0 != SP_Host_Iface_CQ_Push_Cmd((void *)&message, sizeof(message)))
    {
        Log_Write(LOG_LEVEL_ERROR, "exception_event :  push to CQ failed!\n");
    }
}
