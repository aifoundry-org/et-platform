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
    BL2_Report_Event
*/
/***********************************************************************/
#include "bl2_exception.h"
#include "portmacro.h"

/* Local functions */
static void dump_perf_globals_trace(void);
static void dump_power_globals_trace(void);
static void dump_power_states_globals_trace(void);

/*! \def DUMP_THROTTLE_RESIDENCY(throttle_state, residency_struct, buff_ptr, buff_idx)
    \brief Dumps throttle residency to memory
*/
#define DUMP_THROTTLE_RESIDENCY(throttle_state, residency_struct, buff_ptr, buff_idx) \
    get_throttle_residency(throttle_state, &residency_struct);                        \
    memcpy(buff_ptr, &residency_struct, sizeof(residency_struct));                    \
    buff_idx += sizeof(residency_struct);

/*! \def DUMP_POWER_RESIDENCY(power_state, residency_struct, buff_ptr, buff_idx)
    \brief Dumps power residency to memory
*/
#define DUMP_POWER_RESIDENCY(power_state, residency_struct, buff_ptr, buff_idx) \
    get_power_residency(power_state, &residency_struct);                        \
    memcpy(buff_ptr, &residency_struct, sizeof(residency_struct));              \
    buff_idx += sizeof(residency_struct);

/* trap context variable has to be set to true in case of exception. This will
   enable sending exception event to host without using any semaphores */
extern bool is_trap_context;
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
    uint8_t *trace_buf;

    /* It is an exception trap so set trap context to true here*/
    is_trap_context = true;

    /* Disable global interrupts - no nested exceptions */
    portDISABLE_INTERRUPTS();

    /* Dump the arch and chip specific registers. No chip registers are
    currently saved in the context switch code and log the execution stack event to trace */
    trace_buf = Trace_Exception_Dump_Context(stack_frame);

    /* Generate exception event for host */
    BL2_Report_Event(SP_TRACE_GET_ENTRY_OFFSET(trace_buf), SP_RUNTIME_EXCEPT);

    /* Dump performance and power globals to trace */
    dump_perf_globals_trace();
    dump_power_globals_trace();
    dump_power_states_globals_trace();

    Log_Write(LOG_LEVEL_CRITICAL, "SP Spining in Exception handler..\r\n");

    /* No recovery for now - spin forever */
    while (1)
        ;
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
*       None
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
static void dump_perf_globals_trace(void)
{
    struct asic_frequencies_t asic_frequencies = { 0 };
    struct dram_bw_t dram_bw = { 0 };
    struct max_dram_bw_t max_dram_bw = { 0 };
    uint32_t pct_cap = 0;
    uint64_t last_ts = 0;
    uint8_t data_buff[SP_PERF_GLOBALS_SIZE];
    uint64_t buff_idx = 0;

    /* Read the value and copy it to buffer */
    get_module_asic_frequencies(&asic_frequencies);
    memcpy(&data_buff[buff_idx], &asic_frequencies, sizeof(struct asic_frequencies_t));
    buff_idx += sizeof(struct asic_frequencies_t);

    /* Read the value and copy it to buffer */
    get_module_dram_bw(&dram_bw);
    memcpy(&data_buff[buff_idx], &dram_bw, sizeof(struct dram_bw_t));
    buff_idx += sizeof(struct dram_bw_t);

    /* Read the value and copy it to buffer */
    get_module_max_dram_bw(&max_dram_bw);
    memcpy(&data_buff[buff_idx], &max_dram_bw, sizeof(struct max_dram_bw_t));
    buff_idx += sizeof(struct max_dram_bw_t);

    /* Read the value and copy it to buffer */
    get_dram_capacity_percent(&pct_cap);
    memcpy(&data_buff[buff_idx], &pct_cap, sizeof(uint32_t));
    buff_idx += sizeof(uint32_t);

    /* Read the value and copy it to buffer */
    get_last_update_ts(&last_ts);
    memcpy(&data_buff[buff_idx], &last_ts, sizeof(uint64_t));

    /* Dump the data to trace using SP custom event */
    Trace_Custom_Event(Trace_Get_SP_CB(), TRACE_CUSTOM_TYPE_SP_PERF_GLOBALS, data_buff,
                       SP_PERF_GLOBALS_SIZE);
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
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dump_power_globals_trace(void)
{
    struct module_uptime_t module_uptime = { 0 };
    struct module_voltage_t module_voltage = { 0 };
    power_state_e power_state = 0;
    uint8_t tdp_level = 0;
    uint8_t temp = 0;
    uint16_t soc_pwr = 0;
    struct current_temperature_t temperature;
    uint8_t data_buff[SP_POWER_GLOBALS_SIZE];
    uint64_t buff_idx = 0;

    /* Read the value and copy it to buffer */
    get_module_power_state(&power_state);
    memcpy(&data_buff[buff_idx], &power_state, sizeof(power_state_e));
    buff_idx += sizeof(power_state_e);

    /* Read the value and copy it to buffer */
    get_module_tdp_level(&tdp_level);
    data_buff[buff_idx] = tdp_level;
    buff_idx += sizeof(uint8_t);

    /* Read the value and copy it to buffer */
    get_module_current_temperature(&temperature);
    data_buff[buff_idx] = temperature.pmic_sys;
    buff_idx += sizeof(uint8_t);

    /* Read the value and copy it to buffer */
    get_module_soc_power(&soc_pwr);
    memcpy(&data_buff[buff_idx], &soc_pwr, sizeof(uint16_t));
    buff_idx += sizeof(uint16_t);

    /* Read the value and copy it to buffer */
    get_soc_max_temperature(&temp);
    data_buff[buff_idx] = temp;
    buff_idx += sizeof(uint8_t);

    /* Read the value and copy it to buffer */
    get_module_uptime(&module_uptime);
    memcpy(&data_buff[buff_idx], &module_uptime, sizeof(struct module_uptime_t));
    buff_idx += sizeof(struct module_uptime_t);

    /* Read the value and copy it to buffer */
    get_module_voltage(&module_voltage);
    memcpy(&data_buff[buff_idx], &module_voltage, sizeof(struct module_voltage_t));

    /* Dump the data to trace using SP custom event */
    Trace_Custom_Event(Trace_Get_SP_CB(), TRACE_CUSTOM_TYPE_SP_POWER_GLOBALS, data_buff,
                       SP_POWER_GLOBALS_SIZE);
}

/************************************************************************
*
*   FUNCTION
*
*       dump_power_states_globals_trace
*
*   DESCRIPTION
*
*       Dumps important power states globals to trace buffer
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
static void dump_power_states_globals_trace(void)
{
    struct residency_t residency = { 0, 0, 0, 0 };
    uint8_t data_buff[SP_POWER_STATES_GLOBALS_SIZE];
    uint64_t buff_idx = 0;

    /* Read the value and copy it to buffer */
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_UP, residency, &data_buff[buff_idx],
                            buff_idx)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_DOWN, residency, &data_buff[buff_idx],
                            buff_idx)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_THERMAL_DOWN, residency, &data_buff[buff_idx],
                            buff_idx)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_POWER_SAFE, residency, &data_buff[buff_idx],
                            buff_idx)
    DUMP_THROTTLE_RESIDENCY(POWER_THROTTLE_STATE_THERMAL_SAFE, residency, &data_buff[buff_idx],
                            buff_idx)

    /* Read the value and copy it to buffer */
    DUMP_POWER_RESIDENCY(POWER_STATE_MAX_POWER, residency, &data_buff[buff_idx], buff_idx)
    DUMP_POWER_RESIDENCY(POWER_STATE_MANAGED_POWER, residency, &data_buff[buff_idx], buff_idx)
    DUMP_POWER_RESIDENCY(POWER_STATE_SAFE_POWER, residency, &data_buff[buff_idx], buff_idx)
    DUMP_POWER_RESIDENCY(POWER_STATE_LOW_POWER, residency, &data_buff[buff_idx], buff_idx)

    /* Dump the data to trace using SP custom event */
    Trace_Custom_Event(Trace_Get_SP_CB(), TRACE_CUSTOM_TYPE_SP_POWER_STATES_GLOBALS, data_buff,
                       SP_POWER_STATES_GLOBALS_SIZE);
}

/************************************************************************
*
*   FUNCTION
*
*       BL2_Report_Event
*
*   DESCRIPTION
*
*       Generate event to host directly via CQ
*
*   INPUTS
*
*       trace_buffer_offset    Offset in trace buffer
*       event    event id to report
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void BL2_Report_Event(uint64_t trace_buffer_offset, enum event_ids event)
{
    struct event_message_t message;

    /* add details in message header and fill payload */
    FILL_EVENT_HEADER(&message.header, event, sizeof(struct event_message_t))
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, timer_get_ticks_count(), trace_buffer_offset)

    /* Post message to the queue */
    if (0 != SP_Host_Iface_CQ_Push_Cmd((void *)&message, sizeof(message)))
    {
        Log_Write(LOG_LEVEL_ERROR, "BL2_Report_Event: push to CQ failed!\n");
    }
}
