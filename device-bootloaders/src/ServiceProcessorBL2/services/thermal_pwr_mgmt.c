/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file thermal_pwr_mgmt.c
    \brief A C module that abstracts the Power Management services

    Public interfaces:
       init_thermal_pwr_mgmt_service
       update_module_power_state
       update_module_tdp_level
       update_module_temperature_threshold
       update_module_current_temperature
       update_module_soc_power
       update_module_uptime
       update_module_throttle_time
       update_module_power_residency
       update_pmb_stats
       get_module_power_state
       get_module_tdp_level
       get_module_temperature_threshold
       get_module_current_temperature
       get_module_soc_power
       get_module_voltage
       get_asic_voltage
       get_soc_max_temperature
       get_module_uptime
       get_throttle_residency
       get_power_residency
       set_power_event_cb
       set_system_voltages
       Thermal_Pwr_Mgmt_Get_Minion_Temperature
       Thermal_Pwr_Mgmt_Get_System_Temperature
       Thermal_Pwr_Mgmt_Get_Minion_Power
       Thermal_Pwr_Mgmt_Get_NOC_Power
       Thermal_Pwr_Mgmt_Get_SRAM_Power
       Thermal_Pwr_Mgmt_Get_System_Power
       Thermal_Pwr_Mgmt_Get_System_Power_Temp_Stats
       Thermal_Pwr_Mgmt_Init_OP_Stats
*/
/***********************************************************************/
#include <math.h>
#include "config/mgmt_build_config.h"
#include "thermal_pwr_mgmt.h"
#include "perf_mgmt.h"
#include "bl2_pmic_controller.h"
#include "bl2_pvt_controller.h"
#include "bl2_thermal_power_monitor.h"
#include "minion_configuration.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bl2_main.h"
#include "bl_error_code.h"
#include "hwinc/minion_lvdpll_program.h"
#include "bl2_pvt_controller.h"
#include "mem_controller.h"
#include "delays.h"

volatile struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));
volatile struct pmic_power_reg_t g_pmic_power_reg __attribute__((section(".data")));

/* The variable used to hold task's handle */
static TaskHandle_t g_pm_handle;

/* Task entry function */
static void thermal_power_task_entry(void *pvParameters);

/* Task stack memory */
static StackType_t g_pm_task[TT_TASK_STACK_SIZE];

/* The variable used to hold the task's data structure. */
static StaticTask_t g_staticTask_ptr;

/* The PMIC isr callback function, called from PMIC isr. */
static void pmic_isr_callback(uint8_t int_cause);

#if !(FAST_BOOT || TEST_FRAMEWORK)
/* This function ensure voltage stability after changing voltage through pmic.
   It checks the voltage with pvt sensors after updating it. */
static int check_voltage_stability(module_e voltage_type, uint8_t voltage);
#endif

/* The variable used to track power states change time. */
static uint64_t power_state_change_time = 0;

static uint64_t mm_state = MM_STATE_IDLE;

struct soc_power_reg_t
{
    struct residency_t power_up_throttled_states_residency;
    struct residency_t power_down_throttled_states_residency;
    struct residency_t thermal_down_throttled_states_residency;
    struct residency_t power_safe_throttled_states_residency;
    struct residency_t thermal_safe_throttled_states_residency;
    power_state_e module_power_state;
    struct op_stats_t op_stats;
    uint8_t max_temp;
    struct temperature_threshold_t temperature_threshold;
    struct module_uptime_t module_uptime;
    struct asic_voltage_t asic_voltage;
    struct residency_t power_max_residency;
    struct residency_t power_managed_residency;
    struct residency_t power_safe_residency;
    struct residency_t power_low_residency;
    dm_event_isr_callback event_cb;
    power_throttle_state_e power_throttle_state;
    uint8_t active_power_management;
};

struct pmic_power_reg_t
{
    struct module_voltage_t module_voltage;
    struct pmb_stats_t pmb_stats;
    uint16_t soc_pwr_10mW;
    uint8_t module_tdp_level;
    uint8_t soc_temperature;
};

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
}

volatile struct pmic_power_reg_t *get_pmic_power_reg(void)
{
    return &g_pmic_power_reg;
}

// TODO: This needs to updated once we have characterized in Silicon
#define DP_per_Mhz 15

/*! \def FILL_POWER_STATUS(ptr, throttle_st, pwr_st, curr_pwr, curr_temp)
    \brief Help macro to fill up Power Status Struct
*/
#define FILL_POWER_STATUS(ptr, u, v, w, x, y, z) \
    ptr.throttle_state = u;                      \
    ptr.power_state = v;                         \
    ptr.current_power = w;                       \
    ptr.current_temp = x;                        \
    ptr.tgt_freq = y;                            \
    ptr.tgt_voltage = z;

/*! \def SEND_THROTTLE_EVENT(THROTTLE_EVENT_TYPE)
    \brief Sends throttle event to host
*/
#define SEND_THROTTLE_EVENT(THROTTLE_EVENT_TYPE)                                                   \
    if (g_soc_power_reg.event_cb)                                                                  \
    {                                                                                              \
        /* Send the event to the device reporting the time spend in throttling state */            \
        FILL_EVENT_HEADER(&message.header, THROTTLE_EVENT_TYPE, sizeof(struct event_message_t));   \
                                                                                                   \
        FILL_EVENT_PAYLOAD(&message.payload, INFO, 0, end_time - start_time, 0);                   \
                                                                                                   \
        /* call the callback function and post message */                                          \
        g_soc_power_reg.event_cb(0, &message);                                                     \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: event_cb is not initialized\r\n"); \
    }

/*! \def UPDATE_RESIDENCY(RESIDENCY_TYPE)
    \brief Updates residency
*/
#define UPDATE_RESIDENCY(RESIDENCY_TYPE)                              \
    g_soc_power_reg.RESIDENCY_TYPE.cumulative += time_usec;           \
                                                                      \
    /* Update the AVG throttle time */                                \
    if (0 != g_soc_power_reg.RESIDENCY_TYPE.average)                  \
    {                                                                 \
        g_soc_power_reg.RESIDENCY_TYPE.average =                      \
            (g_soc_power_reg.RESIDENCY_TYPE.average + time_usec) / 2; \
    }                                                                 \
    else                                                              \
    {                                                                 \
        g_soc_power_reg.RESIDENCY_TYPE.average = time_usec;           \
    }                                                                 \
                                                                      \
    /* Update the MAX throttle time */                                \
    if (g_soc_power_reg.RESIDENCY_TYPE.maximum < time_usec)           \
    {                                                                 \
        g_soc_power_reg.RESIDENCY_TYPE.maximum = time_usec;           \
    }                                                                 \
                                                                      \
    /* Update the MIN throttle time */                                \
    if (0 == g_soc_power_reg.RESIDENCY_TYPE.minimum)                  \
    {                                                                 \
        g_soc_power_reg.RESIDENCY_TYPE.minimum = time_usec;           \
    }                                                                 \
    else                                                              \
    {                                                                 \
        if (g_soc_power_reg.RESIDENCY_TYPE.minimum > time_usec)       \
        {                                                             \
            g_soc_power_reg.RESIDENCY_TYPE.minimum = time_usec;       \
        }                                                             \
    }

/*! \def PRINT_RESIDENCY(RESIDENCY_TYPE)
    \brief Prints residency
*/
#define PRINT_RESIDENCY(RESIDENCY_TYPE)                                                          \
    Log_Write(LOG_LEVEL_CRITICAL, "\t cumulative = %lu\n",                                       \
              g_soc_power_reg.RESIDENCY_TYPE.cumulative);                                        \
    Log_Write(LOG_LEVEL_CRITICAL, "\t average = %lu\n", g_soc_power_reg.RESIDENCY_TYPE.average); \
    Log_Write(LOG_LEVEL_CRITICAL, "\t maximum = %lu\n", g_soc_power_reg.RESIDENCY_TYPE.maximum); \
    Log_Write(LOG_LEVEL_CRITICAL, "\t minimum = %lu\n", g_soc_power_reg.RESIDENCY_TYPE.minimum);

/*! \def UPPER_POWER_THRESHOLD_GUARDBAND(tdp)
    \brief TDP guard band, (POWER_GUARDBAND_SCALE_FACTOR)% above TDP level,
*          It is the margin above TDP level to allow Power management to
*          move up and down.
*/
#define UPPER_POWER_THRESHOLD_GUARDBAND(tdp) ((tdp * (100 + POWER_GUARDBAND_SCALE_FACTOR)) / 100)

/*! \def LOWER_POWER_THRESHOLD_GUARDBAND(tdp)
    \brief TDP guard band, (POWER_GUARDBAND_SCALE_FACTOR)% below TDP level,
*          It is the margin below TDP level to allow Power management to move down.
*/
#define LOWER_POWER_THRESHOLD_GUARDBAND(tdp) ((tdp * (100 - POWER_GUARDBAND_SCALE_FACTOR)) / 100)

/*! \def GET_POWER_STATE(power_mW)
    \brief Depending on given power returns current power state.
*/
#define GET_POWER_STATE(power_mW, tdp) \
    (power_mW > tdp) ?                 \
        POWER_STATE_MAX_POWER :        \
        (power_mW > SAFE_POWER_THRESHOLD) ? POWER_STATE_MANAGED_POWER : POWER_STATE_SAFE_POWER

/*! \def MAX(x,y)
    \brief Returns max
*/
#define MAX(x, y) x > y ? x : y

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x, y) y == 0 ? x : x < y ? x : y

/*! \def CMA_TEMP_SAMPLE_COUNT
    \brief Device statistics moving average sample count for temperature.
*/
#define CMA_TEMP_SAMPLE_COUNT 5

/*! \def CMA_FREQ_SAMPLE_COUNT
    \brief Device statistics moving average sample count for frequency.
*/
#define CMA_FREQ_SAMPLE_COUNT 5

/*! \def CMA_VOLTAGE_SAMPLE_COUNT
    \brief Device statistics moving average sample count for minion voltage.
*/
#define CMA_VOLTAGE_SAMPLE_COUNT 5

/* Macro to calculate cumulative moving average */
#define CMA(module, current_value, sample_count)                                             \
    do                                                                                       \
    {                                                                                        \
        double cma_temp =                                                                    \
            (current_value > module.avg) ?                                                   \
                ceil((((double)current_value + (double)(module.avg * (sample_count - 1))) /  \
                      sample_count)) :                                                       \
                floor((((double)current_value + (double)(module.avg * (sample_count - 1))) / \
                       sample_count));                                                       \
        module.avg = (uint16_t)cma_temp;                                                     \
    } while (0);

/* Macro to calculate Min, Max values and add to the sum for average value later to be calculated by dividing num of samples */
#define CALC_MIN_MAX(module, current_value)      \
    module.max = MAX(current_value, module.max); \
    module.min = MIN(current_value, module.min);

/* Macro to calculate power by multiplying voltage and current */
#define CALC_POWER(voltage, current) (uint16_t)(voltage * current)

/* Macro to initialize avg, min and max of a stat module */
#define INIT_STAT_VALUE(module) \
    module.avg = 0;             \
    module.max = 0;             \
    module.min = 0xFFFF;

/* Macro to set avg of a stat module */
#define INITIAL_SET_STAT_AVG_VALUE(module, value) module.avg = value;

/* defines for set voltage wait and check loop */
#define PERCENTAGE_DIFFERENCE(a, b) abs(((a - b) * 100) / b)
#define SET_VOLTAGE_TIMEOUT         1000000
#define SET_VOLTAGE_THRESHOLD       5

/* define for a wait and check loop for set voltage function */
#define VALIDATE_VOLTAGE_CHANGE(module, time_out, func, param, val, voltage_mv, status)                         \
    status = ERROR_PMIC_SET_VOLTAGE;                                                                            \
    time_out = timer_get_ticks_count() + pdMS_TO_TICKS(SET_VOLTAGE_TIMEOUT);                                    \
    while (timer_get_ticks_count() < time_out)                                                                  \
    {                                                                                                           \
        func(&param);                                                                                           \
        if (PERCENTAGE_DIFFERENCE(voltage_mv, val) <= SET_VOLTAGE_THRESHOLD)                                    \
        {                                                                                                       \
            status = STATUS_SUCCESS;                                                                            \
            break;                                                                                              \
        }                                                                                                       \
        US_DELAY_GENERIC(50);                                                                                   \
    }                                                                                                           \
    if (status == STATUS_SUCCESS)                                                                               \
    {                                                                                                           \
        Log_Write(                                                                                              \
            LOG_LEVEL_INFO,                                                                                     \
            "VALIDATE_VOLTAGE_CHANGE: %ld cycles consumed stabilizing voltage of module: %d\n",                 \
            (timer_get_ticks_count() - (time_out - pdMS_TO_TICKS(SET_VOLTAGE_TIMEOUT))), module);               \
    }                                                                                                           \
    else                                                                                                        \
    {                                                                                                           \
        Log_Write(                                                                                              \
            LOG_LEVEL_WARNING,                                                                                  \
            "VALIDATE_VOLTAGE_CHANGE: Module %d voltage verification failed: target vol: %d current vol: %d\n", \
            module, voltage_mv, val);                                                                           \
    }

#define VALIDATE_VOLTAGE_CHANGE_PMIC(module, target_voltage, status)                 \
    {                                                                                \
        uint64_t time_out = 0;                                                       \
        uint8_t updated_volt = 0;                                                    \
                                                                                     \
        /* Do voltage verification from PMIC first */                                \
        time_out = timer_get_ticks_count() + pdMS_TO_TICKS(SET_VOLTAGE_TIMEOUT);     \
        do                                                                           \
        {                                                                            \
            /* Get the voltage from PMIC and verify */                               \
            status = pmic_get_voltage(module, &updated_volt);                        \
            if ((status == STATUS_SUCCESS) && (updated_volt == target_voltage))      \
            {                                                                        \
                break;                                                               \
            }                                                                        \
                                                                                     \
            /* Check timeout condition */                                            \
            if ((status == STATUS_SUCCESS) && (timer_get_ticks_count() >= time_out)) \
            {                                                                        \
                status = ERROR_PMIC_SET_VOLTAGE;                                     \
            }                                                                        \
            US_DELAY_GENERIC(50);                                                    \
        } while (status == STATUS_SUCCESS);                                          \
    }

/************************************************************************
*
*   FUNCTION
*
*       update_module_power_state
*
*   DESCRIPTION
*
*       This function updates the power state of the module.
*
*   INPUTS
*
*       power_state_e
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_power_state(power_state_e state)
{
    if (power_state_change_time == 0)
    {
        power_state_change_time = timer_get_ticks_count();
    }
    else
    {
        update_module_power_residency(state, timer_get_ticks_count() - power_state_change_time);
    }

    g_soc_power_reg.module_power_state = state;
    power_state_change_time = timer_get_ticks_count();

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       set_module_active_power_management
*
*   DESCRIPTION
*
*       This function sets Active Power Management of the module.
*
*   INPUTS
*
*       active_power_management_e
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int set_module_active_power_management(active_power_management_e state)
{
    g_soc_power_reg.active_power_management = state;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_power_state
*
*   DESCRIPTION
*
*       This function gets the module power state.
*
*   INPUTS
*
*       *power_state       Pointer to power state variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_power_state(power_state_e *power_state)
{
    *power_state = g_soc_power_reg.module_power_state;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_tdp_level
*
*   DESCRIPTION
*
*       This function updates the TDP level in the Global Power Register.
*
*   INPUTS
*
*       tdp_level_e
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_tdp_level(uint8_t tdp)
{
    if (tdp > POWER_THRESHOLD_HW_CATASTROPHIC)
    {
        Log_Write(LOG_LEVEL_ERROR, "SW TDP can not be over HW power threshold!\n");
        return THERMAL_PWR_MGMT_INVALID_TDP_LEVEL;
    }
    else
    {
        Log_Write(LOG_LEVEL_INFO,
                  "thermal pwr mgmt svc: Update module tdp level threshold to new level: %d\r\n",
                  tdp);
        g_pmic_power_reg.module_tdp_level = tdp;
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_tdp_level
*
*   DESCRIPTION
*
*       This function gets the TDP level of the module as per
*       Global Power Register.
*
*   INPUTS
*
*       *tdp_level         Pointer to tdp level variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_tdp_level(uint8_t *tdp_level)
{
    *tdp_level = g_pmic_power_reg.module_tdp_level;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_temperature_threshold
*
*   DESCRIPTION
*
*       This function updates the temperature threshold of the module.
*
*   INPUTS
*
*       threshold
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_temperature_threshold(uint8_t sw_threshold)
{
    g_soc_power_reg.temperature_threshold.sw_temperature_c = sw_threshold;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_temperature_threshold
*
*   DESCRIPTION
*
*       This function gets the temperature thresholds of the module.
*
*   INPUTS
*
*       *temperature_threshold         Pointer to temperature threshold
*                                      variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold)
{
    *temperature_threshold = g_soc_power_reg.temperature_threshold;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_current_temperature
*
*   DESCRIPTION
*
*       This function gets the current temperature of the module.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                      Return status
*
***********************************************************************/
int update_module_current_temperature(void)
{
    int status = 0;
    uint8_t temperature;

    status = pvt_get_minion_avg_temperature(&temperature);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.soc_temperature = temperature;
        CALC_MIN_MAX(g_soc_power_reg.op_stats.minion.temperature, temperature)
        CMA(g_soc_power_reg.op_stats.minion.temperature, temperature, CMA_TEMP_SAMPLE_COUNT)

        /* TODO: update system temperature stats when it is available */
        CALC_MIN_MAX(g_soc_power_reg.op_stats.system.temperature, 0)

        /* Switch power throttle state only if temperature is above threshold value
           and Active Power Management is enabled*/
        if ((temperature > g_soc_power_reg.temperature_threshold.sw_temperature_c) &&
            (g_soc_power_reg.power_throttle_state < POWER_THROTTLE_STATE_THERMAL_DOWN) &&
            (g_soc_power_reg.active_power_management))
        {
            // Do the thermal throttling
            g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_THERMAL_DOWN;
            xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
        }
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to update temperature\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_current_temperature
*
*   DESCRIPTION
*
*       This function gets the current temperature of the module.
*
*   INPUTS
*
*       *temperature         Pointer to temperature variable
*
*   OUTPUTS
*
*       int                      Return status
*
***********************************************************************/
int get_module_current_temperature(struct current_temperature_t *temperature)
{
    TS_Sample pvt_temperature;
    uint8_t pmic_temperature;

    if (0 != pvt_get_minion_avg_temperature(&pmic_temperature))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        temperature->pmic_sys = pmic_temperature;
    }

    if (0 != pvt_get_minion_avg_low_high_temperature(&pvt_temperature))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        return THERMAL_PWR_MGMT_PVT_ACCESS_FAILED;
    }
    else
    {
        g_pmic_power_reg.soc_temperature = (uint8_t)pvt_temperature.current;
        temperature->minshire_avg = pvt_temperature.current;
        temperature->minshire_high = pvt_temperature.high;
        temperature->minshire_low = pvt_temperature.low;
    }

    if (0 != pvt_get_ioshire_ts_sample(&pvt_temperature))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        return THERMAL_PWR_MGMT_PVT_ACCESS_FAILED;
    }
    else
    {
        temperature->ioshire_current = pvt_temperature.current;
        temperature->ioshire_high = pvt_temperature.high;
        temperature->ioshire_low = pvt_temperature.low;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_soc_power
*
*   DESCRIPTION
*
*       This function gets the current power consumed by the device.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                Return status
*
***********************************************************************/
int update_module_soc_power(void)
{
    uint16_t soc_pwr_10mW = 0;
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };

    if (SUCCESS != pvt_get_minion_avg_low_high_voltage(&minshire_voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to update asic voltage\r\n");
        return THERMAL_PWR_MGMT_PVT_ACCESS_FAILED;
    }

    if (SUCCESS != get_module_voltage(NULL))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to update module voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }

    if (0 != pmic_read_average_soc_power(&soc_pwr_10mW))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get soc average power\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        g_soc_power_reg.op_stats.system.power.avg = soc_pwr_10mW;
    }

    if (0 != pmic_read_instantaneous_soc_power(&soc_pwr_10mW))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get soc instant power\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        g_pmic_power_reg.soc_pwr_10mW = soc_pwr_10mW;
        CALC_MIN_MAX(g_soc_power_reg.op_stats.system.power, soc_pwr_10mW)
    }

    /* Update average, min and max values of Minion, NOC and SRAM powers */
    g_soc_power_reg.op_stats.minion.power.avg = g_pmic_power_reg.pmb_stats.minion.w_out.average;
    g_soc_power_reg.op_stats.minion.power.min = g_pmic_power_reg.pmb_stats.minion.w_out.min;
    g_soc_power_reg.op_stats.minion.power.max = g_pmic_power_reg.pmb_stats.minion.w_out.max;

    g_soc_power_reg.op_stats.noc.power.avg = g_pmic_power_reg.pmb_stats.noc.w_out.average;
    g_soc_power_reg.op_stats.noc.power.min = g_pmic_power_reg.pmb_stats.noc.w_out.min;
    g_soc_power_reg.op_stats.noc.power.max = g_pmic_power_reg.pmb_stats.noc.w_out.max;

    g_soc_power_reg.op_stats.sram.power.avg = g_pmic_power_reg.pmb_stats.sram.w_out.average;
    g_soc_power_reg.op_stats.sram.power.min = g_pmic_power_reg.pmb_stats.sram.w_out.min;
    g_soc_power_reg.op_stats.sram.power.max = g_pmic_power_reg.pmb_stats.sram.w_out.max;

    /* Update average, min and max values of Minion, NOC and SRAM voltages from PVT */
    g_soc_power_reg.op_stats.minion.voltage.avg = minshire_voltage.vdd_mnn.current;
    g_soc_power_reg.op_stats.minion.voltage.min = minshire_voltage.vdd_mnn.low;
    g_soc_power_reg.op_stats.minion.voltage.max = minshire_voltage.vdd_mnn.high;

    g_soc_power_reg.op_stats.noc.voltage.avg = minshire_voltage.vdd_noc.current;
    g_soc_power_reg.op_stats.noc.voltage.min = minshire_voltage.vdd_noc.low;
    g_soc_power_reg.op_stats.noc.voltage.max = minshire_voltage.vdd_noc.high;

    g_soc_power_reg.op_stats.sram.voltage.avg = minshire_voltage.vdd_sram.current;
    g_soc_power_reg.op_stats.sram.voltage.min = minshire_voltage.vdd_sram.low;
    g_soc_power_reg.op_stats.sram.voltage.max = minshire_voltage.vdd_sram.high;

    /* module_tdp_level is in Watts, converting to miliWatts */
    int32_t tdp_level_mW = POWER_IN_MW(g_pmic_power_reg.module_tdp_level);

    /* Update the power state */
    update_module_power_state(GET_POWER_STATE(POWER_10MW_TO_MW(soc_pwr_10mW), tdp_level_mW));

    /* Check if the power management is enabled */
    if (g_soc_power_reg.active_power_management == ACTIVE_POWER_MANAGEMENT_TURN_ON)
    {
        /* Switch to idle power state */
        if (mm_state == MM_STATE_IDLE)
        {
            if (g_soc_power_reg.power_throttle_state != POWER_THROTTLE_STATE_POWER_IDLE)
            {
                /* Log the event */
                Log_Write(LOG_LEVEL_CRITICAL,
                          "Power idle state event, current pwr %u  tdp level %u\n",
                          POWER_10MW_TO_MW(soc_pwr_10mW), tdp_level_mW);

                /* Go to idle state */
                g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_POWER_IDLE;
                xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
            }
        }
        else if ((POWER_10MW_TO_MW(soc_pwr_10mW) > tdp_level_mW) &&
                 (g_soc_power_reg.power_throttle_state < POWER_THROTTLE_STATE_POWER_DOWN))
        {
            /* Log the event */
            Log_Write(LOG_LEVEL_CRITICAL,
                      "Power throttle down event, current pwr %u  tdp level: %u\n",
                      POWER_10MW_TO_MW(soc_pwr_10mW), tdp_level_mW);

            /* Do the power throttling down */
            g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_POWER_DOWN;
            xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
        }

        /* Switch power throttle state only if we are currently in lower priority throttle state */
        else if ((POWER_10MW_TO_MW(soc_pwr_10mW) < tdp_level_mW) &&
                 (g_soc_power_reg.power_throttle_state < POWER_THROTTLE_STATE_POWER_UP))
        {
            /* Log the event */
            Log_Write(LOG_LEVEL_CRITICAL,
                      "Power throttle up event, current pwr %u  tdp level: %u\n",
                      POWER_10MW_TO_MW(soc_pwr_10mW), tdp_level_mW);

            /* Do the power throttling up */
            g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_POWER_UP;
            xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_frequencies
*
*   DESCRIPTION
*
*       This function updates the module frequencies in op stats.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                      Return status
*
***********************************************************************/
int update_module_frequencies(void)
{
    int32_t temp_freq = Get_Minion_Frequency();

    CALC_MIN_MAX(g_soc_power_reg.op_stats.minion.freq, (uint16_t)temp_freq)
    CMA(g_soc_power_reg.op_stats.minion.freq, (uint16_t)temp_freq, CMA_FREQ_SAMPLE_COUNT)

    temp_freq = Get_L2cache_Frequency();
    CALC_MIN_MAX(g_soc_power_reg.op_stats.sram.freq, (uint16_t)temp_freq)
    CMA(g_soc_power_reg.op_stats.sram.freq, (uint16_t)temp_freq, CMA_FREQ_SAMPLE_COUNT)

    temp_freq = Get_NOC_Frequency();
    CALC_MIN_MAX(g_soc_power_reg.op_stats.noc.freq, (uint16_t)temp_freq)
    CMA(g_soc_power_reg.op_stats.noc.freq, (uint16_t)temp_freq, CMA_FREQ_SAMPLE_COUNT)

    return SUCCESS;
}
/************************************************************************
*
*   FUNCTION
*
*       get_module_soc_power
*
*   DESCRIPTION
*
*       This function gets the current power consumed by the device in
*       10 mW steps.
*
*   INPUTS
*
*       soc_pwr_10mw       Pointer to soc power value in 10mW
*
*   OUTPUTS
*
*       int                Return status
*
***********************************************************************/
int get_module_soc_power(uint16_t *soc_pwr_10mw)
{
    *soc_pwr_10mw = g_pmic_power_reg.soc_pwr_10mW;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_voltage
*
*   DESCRIPTION
*
*       This function gets the voltage value for a given voltage domain
*       from PMIC.
*
*   INPUTS
*
*       module_voltage  Pointer to module voltage struct
*
*   OUTPUTS
*
*       int             Return status
*
***********************************************************************/
int get_module_voltage(struct module_voltage_t *module_voltage)
{
    int status;
    uint8_t temp = 0;

    status = pmic_get_voltage(MODULE_DDR, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.ddr = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get DDR voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_L2CACHE, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.l2_cache = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get L2 Cache voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_MAXION, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.maxion = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get Maxion voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_MINION, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.minion = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get Minion voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_PCIE, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.pcie = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get PCIe voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_NOC, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.noc = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get NOC voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_PCIE_LOGIC, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.pcie_logic = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get PCIE Logic voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_VDDQLP, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.vddqlp = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get VDDQLP voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    status = pmic_get_voltage(MODULE_VDDQ, &temp);
    if (status == STATUS_SUCCESS)
    {
        g_pmic_power_reg.module_voltage.vddq = temp;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Unable to get VDDQ voltage from PMIC. Status: %d\r\n",
                  __func__, status);
    }

    if (module_voltage != NULL)
    {
        *module_voltage = g_pmic_power_reg.module_voltage;
    }

    return STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       get_asic_voltage
*
*   DESCRIPTION
*
*       This function gets the voltage value for a given voltage domain
*       from PVT.
*
*   INPUTS
*
*       asic_voltage            Pointer to Module voltage struct
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_asic_voltage(struct asic_voltage_t *asic_voltage)
{
    int status = STATUS_SUCCESS;
    uint8_t temp = 0;
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    MemShire_VM_sample memshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    PShire_VM_sample pshr_voltage = { { 0, 0, 0 }, { 0, 0, 0 } };
    IOShire_VM_sample ioshire_voltage = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

    status = pvt_get_minion_avg_low_high_voltage(&minshire_voltage);
    if (status == STATUS_SUCCESS)
    {
        /* SRAM voltage value is to be stored in l2_cache placeholder as currently no SRAM member is available in asic_voltage */
        g_soc_power_reg.asic_voltage.l2_cache = minshire_voltage.vdd_sram.current;
        g_soc_power_reg.asic_voltage.minion = minshire_voltage.vdd_mnn.current;
        g_soc_power_reg.asic_voltage.noc = minshire_voltage.vdd_noc.current;
        Log_Write(
            LOG_LEVEL_DEBUG,
            "get_asic_voltage: L2 Cache/SRAM Voltage: %d \t minion voltage: %d\t noc voltage: %d\r\n",
            minshire_voltage.vdd_sram.current, minshire_voltage.vdd_mnn.current,
            minshire_voltage.vdd_noc.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get minshire voltage\n");
    }

    status = pvt_get_memshire_avg_low_high_voltage(&memshire_voltage);
    if (status == STATUS_SUCCESS)
    {
        g_soc_power_reg.asic_voltage.ddr = memshire_voltage.vdd_ms.current;
        Log_Write(LOG_LEVEL_DEBUG, "get_asic_voltage: ddr voltage: %d\r\n",
                  memshire_voltage.vdd_ms.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get memshire voltage\n");
    }

    status = pvt_get_ioshire_vm_sample(&ioshire_voltage);
    if (status == STATUS_SUCCESS)
    {
        g_soc_power_reg.asic_voltage.maxion = ioshire_voltage.vdd_mxn.current;

        Log_Write(LOG_LEVEL_DEBUG, "get_asic_voltage: maxion voltage: %d\r\n",
                  ioshire_voltage.vdd_mxn.current);

        g_soc_power_reg.asic_voltage.ioshire_0p75 = ioshire_voltage.vdd_0p75.current;

        Log_Write(LOG_LEVEL_DEBUG, "get_asic_voltage: IOShire 0p75 voltage: %d\r\n",
                  ioshire_voltage.vdd_0p75.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get ioshire voltage\n");
    }

    status = pvt_get_pshire_vm_sample(&pshr_voltage);
    if (status == STATUS_SUCCESS)
    {
        g_soc_power_reg.asic_voltage.pshire_0p75 = pshr_voltage.vdd_pshr.current;

        Log_Write(LOG_LEVEL_DEBUG, "get_asic_voltage: Pshire 0p75 voltage: %d\r\n",
                  pshr_voltage.vdd_pshr.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get pshire voltage\n");
    }

    /* For these values we dont have PVT, so will use PMIC */
    pmic_get_voltage(MODULE_VDDQLP, &temp);
    g_soc_power_reg.asic_voltage.vddqlp = temp;
    pmic_get_voltage(MODULE_VDDQ, &temp);
    g_soc_power_reg.asic_voltage.vddq = temp;

    if (asic_voltage != NULL)
    {
        *asic_voltage = g_soc_power_reg.asic_voltage;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_delta_voltage
*
*   DESCRIPTION
*
*       This function reads the latest voltage values and calculates
*       delta between them.
*
*   INPUTS
*
*       module_type      Type of module
*       delta_voltage    Pointer to delta voltage variable
*
*   OUTPUTS
*
*       int               Return status
*
***********************************************************************/
int get_delta_voltage(module_e module_type, int8_t *delta_voltage)
{
    int status = STATUS_SUCCESS;
    uint8_t pmic_voltage = 0;
    uint16_t pvt_voltage = 0;

    switch (module_type)
    {
        case MODULE_DDR:
            pmic_voltage = g_pmic_power_reg.module_voltage.ddr;
            pvt_voltage = g_soc_power_reg.asic_voltage.ddr;
            break;
        case MODULE_L2CACHE:
            pmic_voltage = g_pmic_power_reg.module_voltage.l2_cache;
            pvt_voltage = g_soc_power_reg.asic_voltage.l2_cache;
            break;
        case MODULE_MAXION:
            pmic_voltage = g_pmic_power_reg.module_voltage.maxion;
            pvt_voltage = g_soc_power_reg.asic_voltage.maxion;
            break;
        case MODULE_MINION:
            pmic_voltage = g_pmic_power_reg.module_voltage.minion;
            pvt_voltage = g_soc_power_reg.asic_voltage.minion;
            break;
        case MODULE_PCIE:
            pmic_voltage = g_pmic_power_reg.module_voltage.pcie;
            pvt_voltage = g_soc_power_reg.asic_voltage.pshire_0p75;
            break;
        case MODULE_NOC:
            pmic_voltage = g_pmic_power_reg.module_voltage.noc;
            pvt_voltage = g_soc_power_reg.asic_voltage.noc;
            break;
        case MODULE_PCIE_LOGIC:
            pmic_voltage = g_pmic_power_reg.module_voltage.pcie_logic;
            pvt_voltage = g_soc_power_reg.asic_voltage.ioshire_0p75;
            break;
        case MODULE_VDDQLP:
            pmic_voltage = g_pmic_power_reg.module_voltage.vddqlp;
            pvt_voltage = g_soc_power_reg.asic_voltage.vddqlp;
            break;
        case MODULE_VDDQ:
            pmic_voltage = g_pmic_power_reg.module_voltage.vddq;
            pvt_voltage = g_soc_power_reg.asic_voltage.vddq;
            break;
        default: {
            status = THERMAL_PWR_MGMT_INVALID_MODULE_TYPE;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        *delta_voltage = (int8_t)(pmic_voltage - pvt_voltage);
        if (*delta_voltage < 0)
        {
            status = THERMAL_PWR_MGMT_INVALID_VOLTAGE;
            Log_Write(LOG_LEVEL_ERROR, "%s: Error: PMIC voltage less than PVT voltage.\r\n",
                      __func__);
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Invalid module type specified. Status: %d\r\n", __func__,
                  status);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       update_pmb_stats
*
*   DESCRIPTION
*
*       This function updates the PMB stats for NOC, SRAM and minion modules.
*
*   INPUTS
*
*       reset                   reset statistics before update
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_pmb_stats(bool reset)
{
    int status = STATUS_SUCCESS;

    if (reset)
    {
        /* reset PMB values from PMIC */
        status = pmic_reset_pmb_stats();
    }

    if (status == STATUS_SUCCESS)
    {
        struct pmb_stats_t stats;

        /* Obtain PMB values from PMIC */
        status = pmic_get_pmb_stats(&stats);

        if (status == STATUS_SUCCESS)
        {
            g_pmic_power_reg.pmb_stats = stats;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*
*       get_soc_max_temperature
*
*   DESCRIPTION
*
*       This function gets module's max temperature
*
*   INPUTS
*
*       *max_temp   Pointer to max temp variable.
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_soc_max_temperature(uint8_t *max_temp)
{
    *max_temp = g_soc_power_reg.max_temp;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_uptime
*
*   DESCRIPTION
*
*       This function updates the cumulative time starting from Device
*       Reset.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_uptime()
{
    uint64_t module_uptime;
    uint64_t seconds;
    uint16_t day;
    uint8_t hours;
    uint8_t minutes;

    // Module Uptime //
    module_uptime = timer_get_ticks_count();

    seconds = (module_uptime / 1000000);
    day = (uint16_t)(seconds / (HOURS_IN_DAY * SECONDS_IN_HOUR));
    seconds = (seconds % (HOURS_IN_DAY * SECONDS_IN_HOUR));
    hours = (uint8_t)(seconds / SECONDS_IN_HOUR);

    seconds %= SECONDS_IN_HOUR;
    minutes = (uint8_t)(seconds / SECONDS_IN_MINUTE);

    g_soc_power_reg.module_uptime.day = day;      //days
    g_soc_power_reg.module_uptime.hours = hours;  //hours
    g_soc_power_reg.module_uptime.mins = minutes; //mins
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_uptime
*
*   DESCRIPTION
*
*       This function gets the cumulative time starting from Device
*       Reset.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_module_uptime(struct module_uptime_t *module_uptime)
{
    *module_uptime = g_soc_power_reg.module_uptime;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_throttle_time
*
*   DESCRIPTION
*
*       This function gets module's throttle time and
*       updates the global variable
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_throttle_time(power_throttle_state_e throttle_state, uint64_t time_usec)
{
    switch (throttle_state)
    {
        case POWER_THROTTLE_STATE_POWER_UP: {
            UPDATE_RESIDENCY(power_up_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_STATE_POWER_DOWN: {
            UPDATE_RESIDENCY(power_down_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_DOWN: {
            UPDATE_RESIDENCY(thermal_down_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_STATE_POWER_SAFE: {
            UPDATE_RESIDENCY(power_safe_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_SAFE: {
            UPDATE_RESIDENCY(thermal_safe_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_STATE_POWER_IDLE:
            /*TODO: add IDLE state residency */
            break;

        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n", throttle_state);
            return THERMAL_PWR_MGMT_UNKNOWN_THROTTLE_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_power_residency
*
*   DESCRIPTION
*
*       This function sets module's power residency and
*       updates the global variable
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_power_residency(power_state_e power_state, uint64_t time_usec)
{
    switch (power_state)
    {
        case POWER_STATE_MAX_POWER: {
            UPDATE_RESIDENCY(power_max_residency)
            break;
        }
        case POWER_STATE_MANAGED_POWER: {
            UPDATE_RESIDENCY(power_managed_residency)
            break;
        }
        case POWER_STATE_SAFE_POWER: {
            UPDATE_RESIDENCY(power_safe_residency)
            break;
        }
        case POWER_STATE_LOW_POWER: {
            UPDATE_RESIDENCY(power_low_residency)
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power state!\n");
            return THERMAL_PWR_MGMT_UNKNOWN_POWER_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_throttle_residency
*
*   DESCRIPTION
*
*       This function gets throttle residency from the global variable.
*
*   INPUTS
*
*       throttle_state      Throttle state
*       residency           Module's throttle residency
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_throttle_residency(power_throttle_state_e throttle_state, struct residency_t *residency)
{
    switch (throttle_state)
    {
        case POWER_THROTTLE_STATE_POWER_UP: {
            *residency = g_soc_power_reg.power_up_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_POWER_DOWN: {
            *residency = g_soc_power_reg.power_down_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_DOWN: {
            *residency = g_soc_power_reg.thermal_down_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_POWER_SAFE: {
            *residency = g_soc_power_reg.power_safe_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_SAFE: {
            *residency = g_soc_power_reg.thermal_safe_throttled_states_residency;
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n", throttle_state);
            return THERMAL_PWR_MGMT_UNKNOWN_THROTTLE_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_power_residency
*
*   DESCRIPTION
*
*       This function gets power residency from the global variable.
*
*   INPUTS
*
*       power_state         Power state
*       residency           Module's power residency
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_power_residency(power_state_e power_state, struct residency_t *residency)
{
    switch (power_state)
    {
        case POWER_STATE_MAX_POWER: {
            *residency = g_soc_power_reg.power_max_residency;
            break;
        }
        case POWER_STATE_MANAGED_POWER: {
            *residency = g_soc_power_reg.power_managed_residency;
            break;
        }
        case POWER_STATE_SAFE_POWER: {
            *residency = g_soc_power_reg.power_safe_residency;
            break;
        }
        case POWER_STATE_LOW_POWER: {
            *residency = g_soc_power_reg.power_low_residency;
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power state!\n");
            return THERMAL_PWR_MGMT_UNKNOWN_POWER_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       set_power_event_cb
*
*   DESCRIPTION
*
*       This function sets power event callback variable.
*
*   INPUTS
*
*       event_cb                  event callback function ptr
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int set_power_event_cb(dm_event_isr_callback event_cb)
{
    g_soc_power_reg.event_cb = event_cb;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       init_thermal_pwr_mgmt_service
*
*   DESCRIPTION
*
*       This function initializes thermal and power management service
*
*   INPUTS
*
*       event_cb                  none
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int init_thermal_pwr_mgmt_service(void)
{
    int status = 0;

    g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_POWER_IDLE;
    g_soc_power_reg.active_power_management = ACTIVE_POWER_MANAGEMENT_TURN_ON;

    /* Create the power management task - use for throttling and DVFS */
    g_pm_handle = xTaskCreateStatic(thermal_power_task_entry, "TT_TASK", TT_TASK_STACK_SIZE, NULL,
                                    TT_TASK_PRIORITY, g_pm_task, &g_staticTask_ptr);
    if (!g_pm_handle)
    {
        MESSAGE_ERROR("Task Creation Failed: Failed to create power management task.\n");
        status = THERMAL_PWR_MGMT_TASK_CREATION_FAILED;
    }

    if (!status)
    {
        /* Set default parameters */
        status = update_module_temperature_threshold(TEMP_THRESHOLD_SW_MANAGED);

        if (!status)
        {
            status = update_module_tdp_level(POWER_THRESHOLD_SW_MANAGED);
        }

        if (!status)
        {
            status = update_module_power_state(POWER_STATE_LOW_POWER);
        }

        if (!status)
        {
            status = pmic_thermal_pwr_cb_init(pmic_isr_callback);
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       set_minion_operating_point
*
*   DESCRIPTION
*
*       This function will set minion operating point according to new
*       frequency. It will calculate voltage according to frequency and
*       apply that voltage to change operating point.
*
*   INPUTS
*
*       new_freq                  new frequency to switch op
*       power_status              power status trace log entry
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int set_minion_operating_point(uint16_t new_freq,
                                      struct trace_event_power_status_t *power_status)
{
    uint8_t hpdpll_mode = 0;
    uint8_t new_voltage = 0;
    int status = SUCCESS;

    if (new_freq == (uint16_t)Get_Minion_Frequency())
    {
        return STATUS_SUCCESS;
    }

    /* If we are using modes round frequency to be multiple of MODE_FREQUENCY_STEP_SIZE */
#if USE_FCW_FOR_LVDPLL == 0
    new_freq = (uint16_t)(((new_freq + (MODE_FREQUENCY_STEP_SIZE / 2)) / MODE_FREQUENCY_STEP_SIZE) *
                          MODE_FREQUENCY_STEP_SIZE);
#endif

    /* TODO: SW-14539: Handling of set frequency in lvdpll mode through minion should be configured  */

    new_voltage = PMIC_MILLIVOLT_TO_HEX(Minion_Get_Voltage_Given_Freq(new_freq),
                                        PMIC_MINION_VOLTAGE_BASE, PMIC_MINION_VOLTAGE_MULTIPLIER,
                                        PMIC_GENERIC_VOLTAGE_DIVIDER);

    if (new_voltage != g_pmic_power_reg.module_voltage.minion)
    {
        status = Thermal_Pwr_Mgmt_Set_Validate_Voltage(MODULE_MINION, new_voltage);
        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to update Minion shire voltage\n");
            return status;
        }

        /* Update voltage in global register*/
        g_pmic_power_reg.module_voltage.minion = new_voltage;
    }

    /* Set L2cache voltage, it is using same clock as minion */
    new_voltage = PMIC_MILLIVOLT_TO_HEX(Minion_Get_L2Cache_Voltage_Given_Freq(new_freq),
                                        PMIC_SRAM_VOLTAGE_BASE, PMIC_SRAM_VOLTAGE_MULTIPLIER,
                                        PMIC_GENERIC_VOLTAGE_DIVIDER);

    if (new_voltage != g_pmic_power_reg.module_voltage.l2_cache)
    {
        status = Thermal_Pwr_Mgmt_Set_Validate_Voltage(MODULE_L2CACHE, new_voltage);
        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to update L2Cache voltage\n");
            return status;
        }

        /* Update voltage in global register*/
        g_pmic_power_reg.module_voltage.l2_cache = new_voltage;
    }

    status = pwr_svc_find_hpdpll_mode(new_freq, &hpdpll_mode);
    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to get hdpll mode\n");
        return status;
    }

    status = Minion_Configure_Hpdpll(hpdpll_mode, Minion_Get_Active_Compute_Minion_Mask());
    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to update minion frequency!\n");
        new_voltage = PMIC_MILLIVOLT_TO_HEX(
            Minion_Get_Voltage_Given_Freq((uint16_t)Get_Minion_Frequency()),
            PMIC_MINION_VOLTAGE_BASE, PMIC_MINION_VOLTAGE_MULTIPLIER, PMIC_GENERIC_VOLTAGE_DIVIDER);
        if (STATUS_SUCCESS != Thermal_Pwr_Mgmt_Set_Validate_Voltage(MODULE_MINION, new_voltage))
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to update shire voltage\n");
        }
        return status;
    }
    Update_Minion_Frequency_Global_Reg(new_freq);

    power_status->tgt_freq = new_freq;
    power_status->tgt_voltage = new_voltage;

    Trace_Power_Status(Trace_Get_SP_CB(), power_status);

    Log_Write(
        LOG_LEVEL_CRITICAL, "new OP: Freq %d MNN voltage %d SRM voltage %d \n", new_freq,
        PMIC_HEX_TO_MILLIVOLT(g_pmic_power_reg.module_voltage.minion, PMIC_MINION_VOLTAGE_BASE,
                              PMIC_MINION_VOLTAGE_MULTIPLIER, PMIC_GENERIC_VOLTAGE_DIVIDER),
        PMIC_HEX_TO_MILLIVOLT(g_pmic_power_reg.module_voltage.l2_cache, PMIC_SRAM_VOLTAGE_BASE,
                              PMIC_SRAM_VOLTAGE_MULTIPLIER, PMIC_GENERIC_VOLTAGE_DIVIDER));
    return status;
}
/************************************************************************
*
*   FUNCTION
*
*       reduce_minion_operating_point
*
*   DESCRIPTION
*
*       This function will algorithmically lower down frequency/voltage
*       of the Minion Shire to compensate for Over-Temperature OR
*       Over-Power operating regions
*
*   INPUTS
*
*       power_status              power status trace log entry
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int reduce_minion_operating_point(struct trace_event_power_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    uint16_t new_freq = (uint16_t)(
        ((Get_Minion_Frequency() - MINION_FREQUENCY_STEP_VALUE) < MINION_FREQUENCY_MIN_LIMIT) ?
            MINION_FREQUENCY_MIN_LIMIT :
            (Get_Minion_Frequency() - MINION_FREQUENCY_STEP_VALUE));

    /* Set the operating point*/
    return set_minion_operating_point(new_freq, power_status);
}

/************************************************************************
*
*   FUNCTION
*
*       increase_minion_operating_point
*
*   DESCRIPTION
*
*       This function will algorithmically increase frequency/voltage
*       of the Minion Shire to hit most efficient operating point
*
*   INPUTS
*
*       power_status              power status trace log entry
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int increase_minion_operating_point(struct trace_event_power_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    uint16_t new_freq = (uint16_t)(
        ((Get_Minion_Frequency() + MINION_FREQUENCY_STEP_VALUE) > MINION_FREQUENCY_MAX_LIMIT) ?
            MINION_FREQUENCY_MAX_LIMIT :
            (Get_Minion_Frequency() + MINION_FREQUENCY_STEP_VALUE));

    /* Set the operating point*/
    return set_minion_operating_point(new_freq, power_status);
}

/************************************************************************
*
*   FUNCTION
*
*       go_to_idle_state
*
*   DESCRIPTION
*
*       This function will set the system to idle frequency
*
*   INPUTS
*
*       power_status              power status to log to trace
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int go_to_idle_state(struct trace_event_power_status_t *power_status)
{
    /* Set the operating point*/
    return set_minion_operating_point(MNN_BOOT_FREQUENCY, power_status);
}

/************************************************************************
*
*   FUNCTION
*
*       go_to_safe_state
*
*   DESCRIPTION
*
*       This function will switch frequency and voltage to
*       safe predefined values
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int go_to_safe_state(power_state_e power_state, power_throttle_state_e throttle_state)
{
    uint8_t current_temperature = DEF_SYS_TEMP_VALUE;
    uint16_t soc_pwr_10mW = 0;
    struct trace_event_power_status_t power_status = { 0 };
    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(SAFE_STATE_FREQUENCY);
    int status = STATUS_SUCCESS;

    /* update module frequency */
    if (SAFE_STATE_FREQUENCY != Get_Minion_Frequency())
    {
        /* TODO: SW-14539: Handling of set frequency in lvdpll mode through minion should be configured  */
        status = Thermal_Pwr_Set_Module_Frequency(PLL_ID_MINION_PLL, SAFE_STATE_FREQUENCY,
                                                  MINION_PLL_USE_STEP_CLOCK);
        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to go to safe state!\n");
        }

        if (new_voltage != g_soc_power_reg.asic_voltage.minion)
        {
            //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
        }
    }

    if (status == STATUS_SUCCESS)
    {
        Update_Minion_Frequency_Global_Reg(SAFE_STATE_FREQUENCY);

        /* Get the current power */
        status = pmic_read_average_soc_power(&soc_pwr_10mW);
        if (status == STATUS_SUCCESS)
        {
            /* Get current temperature */
            status = pvt_get_minion_avg_temperature(&current_temperature);
            if (status == STATUS_SUCCESS)
            {
                FILL_POWER_STATUS(power_status, throttle_state, power_state,
                                  POWER_10MW_TO_W(soc_pwr_10mW), current_temperature,
                                  (uint16_t)SAFE_STATE_FREQUENCY, (uint16_t)new_voltage)

                Trace_Power_Status(Trace_Get_SP_CB(), &power_status);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "thermal pwr mgmt svc error: failed to get temperature\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       trace_power_state_test
*
*   DESCRIPTION
*
*       This function logs power status in trace for test
*
*   INPUTS
*
*       tag               Command tag id
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       cmd               Power throttling command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void trace_power_state_test(uint16_t tag, uint64_t req_start_time, void *cmd)
{
    struct device_mgmt_power_throttle_config_rsp_t dm_rsp;
    const struct device_mgmt_power_throttle_config_cmd_t *pwr_state_cmd =
        (struct device_mgmt_power_throttle_config_cmd_t *)cmd;
    struct trace_event_power_status_t power_status = { 0 };

    pmic_read_average_soc_power(&power_status.current_power);
    power_status.current_power = POWER_10MW_TO_W(power_status.current_power);
    pvt_get_minion_avg_temperature(&power_status.current_temp);
    power_status.power_state = pwr_state_cmd->power_state;

    Trace_Power_Status(Trace_Get_SP_CB(), &power_status);

    /* Update the trace header to make the event visible */
    Trace_Update_SP_Buffer_Header();

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_THROTTLE_POWER_STATE_TEST,
                    timer_get_ticks_count() - req_start_time, DM_STATUS_SUCCESS)

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_power_throttle_config_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "power_throttling_cmd_handler: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       power_throttling
*
*   DESCRIPTION
*
*       This function handles power throttling
*
*   INPUTS
*
*       throttle_state
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void power_throttling(power_throttle_state_e throttle_state)
{
    uint64_t start_time;
    uint64_t end_time;
    uint8_t current_temperature = 0;
    uint16_t avg_pwr_10mW;
    int32_t tdp_level_mW;
    uint8_t throttle_condition_met = 0;
    int status = SUCCESS;
    struct trace_event_power_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == POWER_THROTTLE_STATE_POWER_SAFE)
    {
        go_to_safe_state(g_soc_power_reg.module_power_state, throttle_state);
    }

    status = pvt_get_minion_avg_temperature(&current_temperature);
    if (status == SUCCESS)
    {
        status = pmic_read_average_soc_power(&avg_pwr_10mW);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "thermal pwr mgmt svc error: %d failed to get soc power \r\n", status);
            return;
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "thermal pwr mgmt svc error: %d failed to get soc temperature\r\n", status);
        return;
    }

    /* module_tdp_level is in Watts, converting to miliWatts */
    tdp_level_mW = POWER_IN_MW(g_pmic_power_reg.module_tdp_level);

    while (!throttle_condition_met)
    {
        /* Program the new operating point */
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_POWER_UP: {
                FILL_POWER_STATUS(power_status, throttle_state, g_soc_power_reg.module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                status = increase_minion_operating_point(&power_status);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_DOWN: {
                FILL_POWER_STATUS(power_status, throttle_state, g_soc_power_reg.module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                status = reduce_minion_operating_point(&power_status);
                break;
            }
            case POWER_THROTTLE_STATE_THERMAL_IDLE:
            case POWER_THROTTLE_STATE_POWER_IDLE: {
                FILL_POWER_STATUS(power_status, throttle_state, g_soc_power_reg.module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                status = go_to_idle_state(&power_status);
                break;
            }

            case POWER_THROTTLE_STATE_POWER_SAFE: {
                FILL_POWER_STATUS(power_status, throttle_state, g_soc_power_reg.module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                status = go_to_safe_state(g_soc_power_reg.module_power_state, throttle_state);
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n",
                          throttle_state);
            }
        }

        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "thermal pwr mgmt svc error: %d failed to change operating point\r\n",
                      status);
            return;
        }

        /* Get the current power */
        status = pmic_read_average_soc_power(&avg_pwr_10mW);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
            return;
        }

        /* Check if throttle condition is met */
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_POWER_UP: {
                if ((POWER_10MW_TO_MW(avg_pwr_10mW) > tdp_level_mW) ||
                    (Get_Minion_Frequency() == MINION_FREQUENCY_MAX_LIMIT))
                {
                    throttle_condition_met = 1;
                }
                break;
            }
            case POWER_THROTTLE_STATE_POWER_DOWN:
            case POWER_THROTTLE_STATE_POWER_SAFE:
                if ((POWER_10MW_TO_MW(avg_pwr_10mW) <
                     UPPER_POWER_THRESHOLD_GUARDBAND(tdp_level_mW)) ||
                    (Get_Minion_Frequency() == MINION_FREQUENCY_MIN_LIMIT))
                {
                    throttle_condition_met = 1;
                }
                break;
            case POWER_THROTTLE_STATE_POWER_IDLE:
                throttle_condition_met = 1;
                break;
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n",
                          throttle_state);
            }
        }
    }

    end_time = timer_get_ticks_count();

    /* Update the power state */
    if (g_soc_power_reg.module_power_state != POWER_STATE_MANAGED_POWER)
    {
        update_module_power_state(POWER_STATE_MANAGED_POWER);
    }

    /* Update the throttle time here */
    update_module_throttle_time(throttle_state, end_time - start_time);
}

/************************************************************************
*
*   FUNCTION
*
*       thermal_throttling
*
*   DESCRIPTION
*
*       This function handles thermal throttling
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
void thermal_throttling(power_throttle_state_e throttle_state)
{
    uint64_t start_time;
    uint64_t end_time;
    uint8_t current_temperature = DEF_SYS_TEMP_VALUE;
    struct event_message_t message;
    uint16_t avg_pwr_10mW;
    int status = SUCCESS;
    struct trace_event_power_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == POWER_THROTTLE_STATE_THERMAL_SAFE)
    {
        go_to_safe_state(g_soc_power_reg.module_power_state, throttle_state);
        return;
    }

    status = pvt_get_minion_avg_temperature(&current_temperature);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: %d failed to get temperature\r\n",
                  status);
        return;
    }

    while ((current_temperature > g_soc_power_reg.temperature_threshold.sw_temperature_c) &&
           (g_soc_power_reg.power_throttle_state <= throttle_state))
    {
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_THERMAL_DOWN: {
                /* Add a log to indicate thermal */
                Log_Write(LOG_LEVEL_CRITICAL,
                          "Thermal throttle down event, current temprature: %u, threshold: %d\n",
                          current_temperature,
                          g_soc_power_reg.temperature_threshold.sw_temperature_c);

                /* Get the current power */
                status = pmic_read_average_soc_power(&avg_pwr_10mW);
                if (status != SUCCESS)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "thermal pwr mgmt svc error: %d failed to get soc power\r\n", status);
                    return;
                }

                FILL_POWER_STATUS(power_status, throttle_state, g_soc_power_reg.module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                /* Program the new operating point  */
                status = reduce_minion_operating_point(&power_status);
                if (status != SUCCESS)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "thermal pwr mgmt svc error: %d failed to set operating point\r\n",
                              status);
                    return;
                }
                break;
            }
            case POWER_THROTTLE_STATE_THERMAL_SAFE: {
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n",
                          throttle_state);
            }
        }

        /* TODO: What should be this delay? */
        vTaskDelay(pdMS_TO_TICKS(DELTA_TEMP_UPDATE_PERIOD));

        /* Sample the temperature again */
        if (0 != pvt_get_minion_avg_temperature(&current_temperature))
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
        }
    }

    end_time = timer_get_ticks_count();

    if (g_soc_power_reg.power_throttle_state <= throttle_state)
    {
        g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_THERMAL_IDLE;
        xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
    }

    if (throttle_state == POWER_THROTTLE_STATE_THERMAL_SAFE)
    {
        SEND_THROTTLE_EVENT(THROTTLE_TIME);
    }

    /* Update the throttle time here */
    update_module_throttle_time(POWER_THROTTLE_STATE_THERMAL_DOWN, end_time - start_time);
}

/************************************************************************
*
*   FUNCTION
*
*       thermal_power_task_entry
*
*   DESCRIPTION
*
*       Entry function for power mgmt task
*
*   INPUTS
*
*       pvParameter                Task parameter
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
void thermal_power_task_entry(void *pvParameter)
{
    uint32_t notificationValue;

    (void)pvParameter;

    uint32_t throttle_state = POWER_THROTTLE_STATE_POWER_IDLE;

    while (1)
    {
        while (throttle_state == g_soc_power_reg.power_throttle_state)
        {
            xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);
        }

        throttle_state = g_soc_power_reg.power_throttle_state;

        Log_Write(LOG_LEVEL_INFO, "Power Throttle event received. throttle_state: %d\n",
                  g_soc_power_reg.power_throttle_state);

        switch (g_soc_power_reg.power_throttle_state)
        {
            case POWER_THROTTLE_STATE_THERMAL_SAFE: {
                thermal_throttling(POWER_THROTTLE_STATE_THERMAL_SAFE);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_SAFE: {
                power_throttling(POWER_THROTTLE_STATE_POWER_SAFE);
                break;
            }
            case POWER_THROTTLE_STATE_THERMAL_DOWN: {
                thermal_throttling(POWER_THROTTLE_STATE_THERMAL_DOWN);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_DOWN: {
                power_throttling(POWER_THROTTLE_STATE_POWER_DOWN);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_UP: {
                power_throttling(POWER_THROTTLE_STATE_POWER_UP);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_IDLE: {
                power_throttling(POWER_THROTTLE_STATE_POWER_IDLE);
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state: %d\n",
                          g_soc_power_reg.power_throttle_state);
                break;
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_isr_callback
*
*   DESCRIPTION
*
*       Callback function invoked from PMIC ISR
*
*   INPUTS
*
*       type                  Error type
*       msg                   Error/Event message
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
static void pmic_isr_callback(uint8_t int_cause)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (PMIC_I2C_INT_CTRL_OV_TEMP_GET(int_cause))
    {
        g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_THERMAL_SAFE;
    }
    else if (PMIC_I2C_INT_CTRL_OV_POWER_GET(int_cause))
    {
        g_soc_power_reg.power_throttle_state = POWER_THROTTLE_STATE_POWER_SAFE;
    }

    xTaskNotifyFromISR(g_pm_handle, (uint32_t)int_cause, eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/************************************************************************
*
*   FUNCTION
*
*       dump_power_globals
*
*   DESCRIPTION
*
*       Dumps important power and thermal globals
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
void dump_power_globals(void)
{
    /* Dump power mgmt globals */
    Log_Write(
        LOG_LEVEL_CRITICAL,
        "power_state = %u, tdp_level = %u, temperature = %u c, power = %u W, max_temperature = %u c\n",
        g_soc_power_reg.module_power_state, g_pmic_power_reg.module_tdp_level,
        g_pmic_power_reg.soc_temperature, POWER_10MW_TO_W(g_pmic_power_reg.soc_pwr_10mW),
        g_soc_power_reg.max_temp);

    Log_Write(LOG_LEVEL_CRITICAL, "Module uptime (day:hours:mins): %d:%d:%d\n",
              g_soc_power_reg.module_uptime.day, g_soc_power_reg.module_uptime.hours,
              g_soc_power_reg.module_uptime.mins);

    /* Print power throttle states residency */
    Log_Write(LOG_LEVEL_CRITICAL, "Module power up throttled residency:\n");
    PRINT_RESIDENCY(power_up_throttled_states_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module power down throttled residency:\n");
    PRINT_RESIDENCY(power_down_throttled_states_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module thermal down throttled residency:\n");
    PRINT_RESIDENCY(thermal_down_throttled_states_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module power safe throttled residency:\n");
    PRINT_RESIDENCY(power_safe_throttled_states_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module thermal safe throttled residency:\n");
    PRINT_RESIDENCY(thermal_safe_throttled_states_residency)

    /* Print power states residency */
    Log_Write(LOG_LEVEL_CRITICAL, "Module power max residency:\n");
    PRINT_RESIDENCY(power_max_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module power managed residency:\n");
    PRINT_RESIDENCY(power_managed_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module power safe residency:\n");
    PRINT_RESIDENCY(power_safe_residency)
    Log_Write(LOG_LEVEL_CRITICAL, "Module power low residency:\n");
    PRINT_RESIDENCY(power_low_residency)

    Log_Write(
        LOG_LEVEL_CRITICAL,
        "Module Voltages (mV) :  ddr = %u , l2_cache = %u, maxion = %u, minion = %u, pshire = %u, noc = %u, ioshire = %u, vddqlp = %u, vddq = %u\n",
        g_soc_power_reg.asic_voltage.ddr, g_soc_power_reg.asic_voltage.l2_cache,
        g_soc_power_reg.asic_voltage.maxion, g_soc_power_reg.asic_voltage.minion,
        g_soc_power_reg.asic_voltage.pshire_0p75, g_soc_power_reg.asic_voltage.noc,
        g_soc_power_reg.asic_voltage.ioshire_0p75, g_soc_power_reg.asic_voltage.vddqlp,
        g_soc_power_reg.asic_voltage.vddq);
}

/************************************************************************
*
*   FUNCTION
*
*       set_system_voltages
*
*   DESCRIPTION
*
*       Set system voltages
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
void set_system_voltages(void)
{
    uint8_t voltage = 0;
    /* Setting the Neigh voltages */
    pmic_set_voltage(MODULE_MINION, MINION_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_MINION, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding Minion -> 500mV(0x%X)\n", voltage);

    /* Setting the L2 cache voltages */
    pmic_set_voltage(MODULE_L2CACHE, SRAM_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_L2CACHE, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding SRAM   -> 750mV(0x%X)\n", voltage);

    /* Setting the NOC voltages */
    pmic_set_voltage(MODULE_NOC, NOC_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_NOC, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding NOC    -> 450mV(0x%X)\n", voltage);

    /* Setting the DDR voltages */
    pmic_set_voltage(MODULE_DDR, DDR_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_DDR, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding DDR    -> 800mV(0x%X)\n", voltage);

    /* Setting the MAXION voltages */
    pmic_set_voltage(MODULE_MAXION, MXN_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_MAXION, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding MAXION -> 600mV(0x%X)\n", voltage);
}

/************************************************************************
*
*   FUNCTION
*
*       print_system_operating_point
*
*   DESCRIPTION
*
*       Prints system operating point
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
void print_system_operating_point(void)
{
    uint32_t freq;
    uint16_t soc_pwr_10mW = 0;
    uint8_t pmic_vol;
    TS_Sample temperature;

    Log_Write(LOG_LEVEL_INFO,
              "SHIRE       \t\tFreq          \t\tVoltage          \t\tTemperature\r\n");
    Log_Write(
        LOG_LEVEL_INFO,
        "---------------------------------------------------------------------------------------------\r\n");

    /* Neigh */
    freq = (uint32_t)Get_Minion_Frequency();
    pvt_get_minion_avg_low_high_temperature(&temperature);
    pmic_get_voltage(MODULE_MINION, &pmic_vol);
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    pvt_get_minion_avg_low_high_voltage(&minshire_voltage);
    Log_Write(LOG_LEVEL_INFO, "NEIGH       \t\tHPDPLL4 %dMHz\t\t%dmV (%dmV)\t\t%dC,[%dC,%dC]\r\n",
              freq,
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_MINION_VOLTAGE_BASE,
                                    PMIC_MINION_VOLTAGE_MULTIPLIER, PMIC_GENERIC_VOLTAGE_DIVIDER),
              minshire_voltage.vdd_mnn.current, temperature.current, temperature.low,
              temperature.high);

    /* Sram */
    pmic_get_voltage(MODULE_L2CACHE, &pmic_vol);
    Log_Write(LOG_LEVEL_INFO, "SRAM        \t\tHPDPLL4 %dMHz\t\t%dmV (%dmV)\t\t/\r\n", freq,
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_SRAM_VOLTAGE_BASE, PMIC_SRAM_VOLTAGE_MULTIPLIER,
                                    PMIC_GENERIC_VOLTAGE_DIVIDER),
              minshire_voltage.vdd_sram.current);

    /* NOC */
    get_pll_frequency(PLL_ID_SP_PLL_2, &freq);
    pmic_get_voltage(MODULE_NOC, &pmic_vol);
    Log_Write(LOG_LEVEL_INFO, "NOC         \t\tHPDPLL2 %dMHz\t\t%dmV (%dmV)\t\t/\r\n", freq,
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_NOC_VOLTAGE_BASE, PMIC_NOC_VOLTAGE_MULTIPLIER,
                                    PMIC_GENERIC_VOLTAGE_DIVIDER),
              minshire_voltage.vdd_noc.current);

    /* MemShire */
    freq = get_memshire_frequency();
    MemShire_VM_sample memshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    pvt_get_memshire_avg_low_high_voltage(&memshire_voltage);
    pmic_get_voltage(MODULE_DDR, &pmic_vol);
    Log_Write(LOG_LEVEL_INFO, "MEMSHIRE    \t\tHPDPLLM %dMHz\t\t%dmV (%dmV)\t\t/\r\n", freq,
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_DDR_VOLTAGE_BASE, PMIC_DDR_VOLTAGE_MULTIPLIER,
                                    PMIC_GENERIC_VOLTAGE_DIVIDER),
              memshire_voltage.vdd_ms.current);

    /* IOShire SPIO */
    get_pll_frequency(PLL_ID_SP_PLL_0, &freq);
    pvt_get_ioshire_ts_sample(&temperature);
    IOShire_VM_sample ioshire_voltage = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
    pvt_get_ioshire_vm_sample(&ioshire_voltage);
    Log_Write(LOG_LEVEL_INFO, "IOSHIRE SPIO\t\tHPDPLL0 %dMHz\t\t/           \t\t%dC,[%dC,%dC]\r\n",
              freq, temperature.current, temperature.low, temperature.high);

    /* IOShire PU */
    get_pll_frequency(PLL_ID_SP_PLL_1, &freq);
    Log_Write(LOG_LEVEL_INFO, "        PU  \t\tHPDPLL1 %dMHz\t\t      (%dmV)\t\t/\r\n", freq,
              ioshire_voltage.vdd_0p75.current);

    /* MAXION */
    pmic_get_voltage(MODULE_MAXION, &pmic_vol);
    Log_Write(LOG_LEVEL_INFO, "MAXION      \t\t/            \t\t%dmV (%dmV)\t\t/\r\n",
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_MAXION_VOLTAGE_BASE,
                                    PMIC_MAXION_VOLTAGE_MULTIPLIER, PMIC_GENERIC_VOLTAGE_DIVIDER),
              ioshire_voltage.vdd_mxn.current);

    /* PSHIRE */
    get_pll_frequency(PLL_ID_PSHIRE, &freq);
    PShire_VM_sample pshr_voltage = { { 0, 0, 0 }, { 0, 0, 0 } };
    pvt_get_pshire_vm_sample(&pshr_voltage);
    pmic_get_voltage(MODULE_PCIE, &pmic_vol);
    Log_Write(LOG_LEVEL_INFO, "PSHIRE      \t\tHPDPLLP %dMHz\t\t%dmV (%dmV)\t\t/\r\n", freq,
              PMIC_HEX_TO_MILLIVOLT(pmic_vol, PMIC_PCIE_VOLTAGE_BASE, PMIC_PCIE_VOLTAGE_MULTIPLIER,
                                    PMIC_PCIE_VOLTAGE_DIVIDER),
              pshr_voltage.vdd_pshr.current);

    /* Card Power */
    pmic_read_average_soc_power(&soc_pwr_10mW);
    Log_Write(LOG_LEVEL_INFO, "AVERAGE CARD POWER: %d W \n", POWER_10MW_TO_W(soc_pwr_10mW));
    Log_Write(
        LOG_LEVEL_INFO,
        "---------------------------------------------------------------------------------------------\r\n");
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_Minion_Temperature
*
*   DESCRIPTION
*
*       This function is used to get minion temperature
*
*   INPUTS
*
*       temp pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_Minion_Temperature(uint64_t *temp)
{
    *temp = g_soc_power_reg.op_stats.minion.temperature.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_System_Temperature
*
*   DESCRIPTION
*
*       This function is used to get system temperature
*
*   INPUTS
*
*       temp pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_System_Temperature(uint64_t *temp)
{
    *temp = g_soc_power_reg.op_stats.system.temperature.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_Minion_Power
*
*   DESCRIPTION
*
*       This function returns minion power consumption
*
*   INPUTS
*
*       power pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_Minion_Power(uint64_t *power)
{
    /* voltage value which can be used to calculate power when current
       value is available. */
    *power = g_soc_power_reg.op_stats.minion.power.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_NOC_Power
*
*   DESCRIPTION
*
*       This function returns noc power consumption
*
*   INPUTS
*
*       power pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_NOC_Power(uint64_t *power)
{
    /* voltage value which can be used to calculate power when current
       value is available. */
    *power = g_soc_power_reg.op_stats.noc.power.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_SRAM_Power
*
*   DESCRIPTION
*
*       This function returns sram power consumption
*
*   INPUTS
*
*       power pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_SRAM_Power(uint64_t *power)
{
    /* voltage value which can be used to calculate power when current
    value is available. */
    *power = g_soc_power_reg.op_stats.sram.power.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_System_Power
*
*   DESCRIPTION
*
*       This function returns system power consumption
*
*   INPUTS
*
*       power pointer to store temperature value
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_System_Power(uint64_t *power)
{
    *power = g_soc_power_reg.op_stats.system.power.avg;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Get_OP_Stats
*
*   DESCRIPTION
*
*       This function returns temperature and power consumption for
*       individual component System, Minion, SRAM and NOC.
*
*   INPUTS
*
*       stats pointer to get all module stats
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Get_System_Power_Temp_Stats(struct op_stats_t *stats)
{
    *stats = g_soc_power_reg.op_stats;
    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Init_OP_Stats
*
*   DESCRIPTION
*
*       This function initialize operating point stats to theur current values.
*
*   INPUTS
*
*       stats pointer to get all module stats
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Init_OP_Stats(void)
{
    int status = STATUS_SUCCESS;
    uint8_t tmp_val;
    uint16_t soc_pwr_10mW;
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };

    /* initialize minion temperature values in op stats */
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.minion.temperature)
    /* initialize system temperature values in op stats */
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.system.temperature)
    /* Initialize power min, max and avg values */
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.minion.power)
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.sram.power)
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.noc.power)
    /* Initialize voltage min, max andd avg values */
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.minion.voltage)
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.sram.voltage)
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.noc.voltage)
    /* initialize op stats average power */
    INIT_STAT_VALUE(g_soc_power_reg.op_stats.system.power)

    /* Reset all PVT sensors */
    pvt_hilo_reset();

    /* read temperature values from pvt */
    status = pvt_get_minion_avg_temperature(&tmp_val);
    if (status == STATUS_SUCCESS)
    {
        /* initialize temperature valuesin op stats */
        INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.minion.temperature, tmp_val)

        /* TODO: PMIC is currently reporting system temperature as 0. This is to be validated once fixed */
        status = pmic_get_temperature(&tmp_val);

        /* Update system temperature */
        if (status == STATUS_SUCCESS)
        {
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.system.temperature, tmp_val)

            /* Update PMB stats */
            status = update_pmb_stats(true);
        }

        if (status == STATUS_SUCCESS)
        {
            /* Initialize power avg values */
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.minion.power,
                                       g_pmic_power_reg.pmb_stats.minion.w_out.average)
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.sram.power,
                                       g_pmic_power_reg.pmb_stats.sram.w_out.average)
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.noc.power,
                                       g_pmic_power_reg.pmb_stats.noc.w_out.average)

            /* Update PVT stats */
            status = pvt_get_minion_avg_low_high_voltage(&minshire_voltage);
        }

        if (status == STATUS_SUCCESS)
        {
            /* Initialize voltage avg values */
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.minion.voltage,
                                       minshire_voltage.vdd_mnn.current)
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.sram.voltage,
                                       minshire_voltage.vdd_sram.current)
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.noc.voltage,
                                       minshire_voltage.vdd_noc.current)

            /* Read card average power */
            status = pmic_read_average_soc_power(&soc_pwr_10mW);
        }

        if (status == STATUS_SUCCESS)
        {
            /* initialize op stats with average power in mW */
            INITIAL_SET_STAT_AVG_VALUE(g_soc_power_reg.op_stats.system.power, soc_pwr_10mW)
        }
    }

    return status;
}

#if !(FAST_BOOT || TEST_FRAMEWORK)
/************************************************************************
*
*   FUNCTION
*
*       check_voltage_stability
*
*   DESCRIPTION
*
*       This function ensure voltage stability after changing voltage through pmic.
*       It checks the voltage with pvt sensors after updating it.
*
*   INPUTS
*
*       voltage_type      voltage type to be set
*       voltage           voltage value to be set
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
static int check_voltage_stability(module_e voltage_type, uint8_t voltage)
{
    int32_t status;
    uint64_t time_end = 0;
    uint16_t voltage_mv = 0;

    switch (voltage_type)
    {
        case MODULE_DDR:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_DDR_VOLTAGE_BASE,
                                                         PMIC_DDR_VOLTAGE_MULTIPLIER,
                                                         PMIC_GENERIC_VOLTAGE_DIVIDER);
            MemShire_VM_sample memshire_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_memshire_avg_low_high_voltage,
                                    memshire_voltage, memshire_voltage.vdd_ms.current, voltage_mv,
                                    status)
            break;
        case MODULE_L2CACHE:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_SRAM_VOLTAGE_BASE,
                                                         PMIC_SRAM_VOLTAGE_MULTIPLIER,
                                                         PMIC_GENERIC_VOLTAGE_DIVIDER);
            MinShire_VM_sample minshire_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_minion_avg_low_high_voltage,
                                    minshire_voltage, minshire_voltage.vdd_sram.current, voltage_mv,
                                    status)
            break;
        case MODULE_MAXION:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_MAXION_VOLTAGE_BASE,
                                                         PMIC_MAXION_VOLTAGE_MULTIPLIER,
                                                         PMIC_GENERIC_VOLTAGE_DIVIDER);
            IOShire_VM_sample ioshire_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_ioshire_vm_sample,
                                    ioshire_voltage, ioshire_voltage.vdd_mxn.current, voltage_mv,
                                    status)
            break;
        case MODULE_MINION:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_MINION_VOLTAGE_BASE,
                                                         PMIC_MINION_VOLTAGE_MULTIPLIER,
                                                         PMIC_GENERIC_VOLTAGE_DIVIDER);
            MinShire_VM_sample mnn_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_minion_avg_low_high_voltage,
                                    mnn_voltage, mnn_voltage.vdd_mnn.current, voltage_mv, status)
            break;
        case MODULE_NOC:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_NOC_VOLTAGE_BASE,
                                                         PMIC_NOC_VOLTAGE_MULTIPLIER,
                                                         PMIC_GENERIC_VOLTAGE_DIVIDER);
            MinShire_VM_sample noc_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_minion_avg_low_high_voltage,
                                    noc_voltage, noc_voltage.vdd_noc.current, voltage_mv, status)
            break;
        case MODULE_PCIE_LOGIC:
            voltage_mv = (uint16_t)PMIC_HEX_TO_MILLIVOLT(voltage, PMIC_PCIE_LOGIC_VOLTAGE_BASE,
                                                         PMIC_PCIE_LOGIC_VOLTAGE_MULTIPLIER,
                                                         PMIC_PCIE_LOGIC_VOLTAGE_DIVIDER);
            IOShire_VM_sample pcie_logic_voltage = { 0 };
            VALIDATE_VOLTAGE_CHANGE(voltage_type, time_end, pvt_get_ioshire_vm_sample,
                                    pcie_logic_voltage, pcie_logic_voltage.vdd_0p75.current,
                                    voltage_mv, status)
            break;
        case MODULE_PCIE: /* pvt is reporting wrong pcie voltage for PCIE */
        case MODULE_VDDQLP:
        case MODULE_VDDQ:
            /* pvt sensors for these modules are not available */
            status = STATUS_SUCCESS;
            break;
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Error invalid voltage type to set Voltage");
            status = ERROR_PMIC_I2C_INVALID_VOLTAGE_TYPE;
            break;
        }
    }

    return status;
}
#endif

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Set_Validate_Voltage
*
*   DESCRIPTION
*
*       This function sets voltage through pmic and then check for voltage stability.
*
*   INPUTS
*
*       voltage_type      voltage type to be set
*       voltage           voltage value to be set
*
*   OUTPUTS
*
*       status of function call success/error
*
***********************************************************************/
int Thermal_Pwr_Mgmt_Set_Validate_Voltage(module_e voltage_type, uint8_t voltage)
{
#if !(FAST_BOOT || TEST_FRAMEWORK)
    int32_t status = STATUS_SUCCESS;
    uint8_t retries = 3;

    /* Enter critical section - Prevents the calling task to not to schedule out.
    While the voltage is being changed on a module, the transactions cannot happen. */
    portENTER_CRITICAL();

    /* TODO: SW-18770: Remove the retries once the issue is resolved. */
    while (retries)
    {
        status = pmic_set_voltage(voltage_type, voltage);
        if (status == STATUS_SUCCESS)
        {
            /* Do the voltage validation from PMIC */
            VALIDATE_VOLTAGE_CHANGE_PMIC(voltage_type, voltage, status)
        }

        /* Do voltage verification through PVT */
        if (status == STATUS_SUCCESS)
        {
            status = check_voltage_stability(voltage_type, voltage);
            if (status == STATUS_SUCCESS)
            {
                /* Voltage is stable, no need to retry now */
                retries = 0;
            }
            else
            {
                Log_Write(LOG_LEVEL_WARNING,
                          "Unable to validate PVT 0x%x voltage for module %d. Retrying.\r\n",
                          voltage, voltage_type);

                /* Decrease the count for retries */
                retries--;
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "Unable to validate PMIC 0x%x voltage for module %d.\r\n",
                      voltage, voltage_type);
        }
    }

    /* Exit critical section */
    portEXIT_CRITICAL();

    return status;
#else
    return pmic_set_voltage(voltage_type, voltage);
#endif
}

/************************************************************************
*
*   FUNCTION
*
*       Thermal_Pwr_Mgmt_Update_MM_State
*
*   DESCRIPTION
*
*       This function sets master minion state either IDLE or BUSY.
*
*   INPUTS
*
*       state    State of the MM to be set to
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Thermal_Pwr_Mgmt_Update_MM_State(uint64_t state)
{
    /* update Master Minion(MM) state*/
    mm_state = state;
}
