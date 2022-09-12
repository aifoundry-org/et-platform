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
#include "minion_configuration.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bl2_main.h"
#include "bl_error_code.h"
#include "hwinc/minion_lvdpll_program.h"
#include "bl2_pvt_controller.h"
#include "mem_controller.h"
#include "delays.h"

struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));

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

/* The variable used to track power states change time. */
static uint64_t power_state_change_time = 0;

struct soc_power_reg_t
{
    struct residency_t power_up_throttled_states_residency;
    struct residency_t power_down_throttled_states_residency;
    struct residency_t thermal_down_throttled_states_residency;
    struct residency_t power_safe_throttled_states_residency;
    struct residency_t thermal_safe_throttled_states_residency;
    power_state_e module_power_state;
    uint8_t module_tdp_level;
    uint8_t soc_temperature;
    struct op_stats_t op_stats;
    uint16_t soc_pwr_10mW;
    uint8_t max_temp;
    struct temperature_threshold_t temperature_threshold;
    struct module_uptime_t module_uptime;
    struct module_voltage_t module_voltage;
    struct residency_t power_max_residency;
    struct residency_t power_managed_residency;
    struct residency_t power_safe_residency;
    struct residency_t power_low_residency;
    struct pmb_stats_t pmb_stats;
    dm_event_isr_callback event_cb;
    power_throttle_state_e power_throttle_state;
    uint8_t active_power_management;
};

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
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
    if (get_soc_power_reg()->event_cb)                                                             \
    {                                                                                              \
        /* Send the event to the device reporting the time spend in throttling state */            \
        FILL_EVENT_HEADER(&message.header, THROTTLE_EVENT_TYPE, sizeof(struct event_message_t));   \
                                                                                                   \
        FILL_EVENT_PAYLOAD(&message.payload, INFO, 0, end_time - start_time, 0);                   \
                                                                                                   \
        /* call the callback function and post message */                                          \
        get_soc_power_reg()->event_cb(0, &message);                                                \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: event_cb is not initialized\r\n"); \
    }

/*! \def UPDATE_RESIDENCY(RESIDENCY_TYPE)
    \brief Updates residency
*/
#define UPDATE_RESIDENCY(RESIDENCY_TYPE)                                   \
    get_soc_power_reg()->RESIDENCY_TYPE.cumulative += time_usec;           \
                                                                           \
    /* Update the AVG throttle time */                                     \
    if (0 != get_soc_power_reg()->RESIDENCY_TYPE.average)                  \
    {                                                                      \
        get_soc_power_reg()->RESIDENCY_TYPE.average =                      \
            (get_soc_power_reg()->RESIDENCY_TYPE.average + time_usec) / 2; \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        get_soc_power_reg()->RESIDENCY_TYPE.average = time_usec;           \
    }                                                                      \
                                                                           \
    /* Update the MAX throttle time */                                     \
    if (get_soc_power_reg()->RESIDENCY_TYPE.maximum < time_usec)           \
    {                                                                      \
        get_soc_power_reg()->RESIDENCY_TYPE.maximum = time_usec;           \
    }                                                                      \
                                                                           \
    /* Update the MIN throttle time */                                     \
    if (0 == get_soc_power_reg()->RESIDENCY_TYPE.minimum)                  \
    {                                                                      \
        get_soc_power_reg()->RESIDENCY_TYPE.minimum = time_usec;           \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        if (get_soc_power_reg()->RESIDENCY_TYPE.minimum > time_usec)       \
        {                                                                  \
            get_soc_power_reg()->RESIDENCY_TYPE.minimum = time_usec;       \
        }                                                                  \
    }

/*! \def PRINT_RESIDENCY(RESIDENCY_TYPE)
    \brief Prints residency
*/
#define PRINT_RESIDENCY(RESIDENCY_TYPE)                                                         \
    Log_Write(LOG_LEVEL_CRITICAL, "\t cumulative = %lu\n",                                      \
              soc_power_reg->RESIDENCY_TYPE.cumulative);                                        \
    Log_Write(LOG_LEVEL_CRITICAL, "\t average = %lu\n", soc_power_reg->RESIDENCY_TYPE.average); \
    Log_Write(LOG_LEVEL_CRITICAL, "\t maximum = %lu\n", soc_power_reg->RESIDENCY_TYPE.maximum); \
    Log_Write(LOG_LEVEL_CRITICAL, "\t minimum = %lu\n", soc_power_reg->RESIDENCY_TYPE.minimum);

/*! \def MAX_POWER_THRESHOLD_GUARDBAND(tdp)
    \brief TDP guard band, (power_scale_factor)% above TDP level,
*          It is the margin above TDP level to allow Power management to
*          move up and down.
*/
#define MAX_POWER_THRESHOLD_GUARDBAND(tdp) ((tdp * (100 + POWER_SCALE_FACTOR)) / 100)

/*! \def GET_POWER_STATE(power_mW)
    \brief Depending on given power returns current power state.
*/
#define GET_POWER_STATE(power_mW, tdp)                \
    (power_mW > MAX_POWER_THRESHOLD_GUARDBAND(tdp)) ? \
        POWER_STATE_MAX_POWER :                       \
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

/* Macro to calculate cumulative moving average */
#define CMA(module, current_value, sample_count)                                         \
    double cma_temp =                                                                    \
        (current_value > module.avg) ?                                                   \
            ceil((((double)current_value + (double)(module.avg * (sample_count - 1))) /  \
                  sample_count)) :                                                       \
            floor((((double)current_value + (double)(module.avg * (sample_count - 1))) / \
                   sample_count));                                                       \
    module.avg = (uint16_t)cma_temp;

/* Macro to calculate Min, Max values and add to the sum for average value later to be calculated by dividing num of samples */
#define CALC_MIN_MAX(module, current_value)      \
    module.max = MAX(current_value, module.max); \
    module.min = MIN(current_value, module.min);

/* Macro to calculate power by multiplying voltage and current */
#define CALC_POWER(voltage, current) (uint16_t)(voltage * current)

/* Macro to initialize avg, min and max of a stat module */
#define INIT_STAT_VALUE(module, value) \
    module.avg = value;                \
    module.max = 0;                    \
    module.min = 0xFFFF;

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

    get_soc_power_reg()->module_power_state = state;
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
    get_soc_power_reg()->active_power_management = state;

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
    *power_state = get_soc_power_reg()->module_power_state;
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
        get_soc_power_reg()->module_tdp_level = tdp;
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
    *tdp_level = get_soc_power_reg()->module_tdp_level;
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
    get_soc_power_reg()->temperature_threshold.sw_temperature_c = sw_threshold;

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
    *temperature_threshold = get_soc_power_reg()->temperature_threshold;
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
    struct temperature_threshold_t temperature_threshold;

    status = pvt_get_minion_avg_temperature(&temperature);
    if (status == STATUS_SUCCESS)
    {
        get_soc_power_reg()->soc_temperature = temperature;
        CALC_MIN_MAX(get_soc_power_reg()->op_stats.minion.temperature, temperature)
        CMA(get_soc_power_reg()->op_stats.minion.temperature, temperature, CMA_TEMP_SAMPLE_COUNT)

        /*TODO: PMIC is currently reporting system temperature as 0. This is to be validated once fixed */
        status = pmic_get_temperature(&temperature);
    }

    /* Update system temperature */
    if (status == STATUS_SUCCESS)
    {
        CALC_MIN_MAX(get_soc_power_reg()->op_stats.system.temperature, temperature)
        CMA(get_soc_power_reg()->op_stats.system.temperature, temperature, CMA_TEMP_SAMPLE_COUNT)
        status = get_module_temperature_threshold(&temperature_threshold);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Switch power throttle state only if we are currently in lower priority throttle
            state and Active Power Management is enabled*/
        if (temperature > (temperature_threshold.sw_temperature_c) &&
            (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_STATE_THERMAL_DOWN) &&
            (get_soc_power_reg()->active_power_management))
        {
            // Do the thermal throttling
            get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_THERMAL_DOWN;
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
        get_soc_power_reg()->soc_temperature = (uint8_t)pvt_temperature.current;
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
        get_soc_power_reg()->op_stats.system.power.avg = soc_pwr_10mW;
    }

    if (0 != pmic_read_instantaneous_soc_power(&soc_pwr_10mW))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get soc instant power\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->soc_pwr_10mW = soc_pwr_10mW;
        CALC_MIN_MAX(get_soc_power_reg()->op_stats.system.power, soc_pwr_10mW)
    }

    /* Update average, min and max values of Minion, NOC and SRAM powers */
    get_soc_power_reg()->op_stats.minion.power.avg =
        get_soc_power_reg()->pmb_stats.minion.w_out.average;
    get_soc_power_reg()->op_stats.minion.power.min =
        get_soc_power_reg()->pmb_stats.minion.w_out.min;
    get_soc_power_reg()->op_stats.minion.power.max =
        get_soc_power_reg()->pmb_stats.minion.w_out.max;

    get_soc_power_reg()->op_stats.noc.power.avg = get_soc_power_reg()->pmb_stats.noc.w_out.average;
    get_soc_power_reg()->op_stats.noc.power.min = get_soc_power_reg()->pmb_stats.noc.w_out.min;
    get_soc_power_reg()->op_stats.noc.power.max = get_soc_power_reg()->pmb_stats.noc.w_out.max;

    get_soc_power_reg()->op_stats.sram.power.avg =
        get_soc_power_reg()->pmb_stats.sram.w_out.average;
    get_soc_power_reg()->op_stats.sram.power.min = get_soc_power_reg()->pmb_stats.sram.w_out.min;
    get_soc_power_reg()->op_stats.sram.power.max = get_soc_power_reg()->pmb_stats.sram.w_out.max;

#if 0 /* TODO: SW-13951: Power throttling causes a hang on silicon under stress */
    /* module_tdp_level is in Watts, converting to miliWatts */
    int32_t tdp_level_mW = get_soc_power_reg()->module_tdp_level * 1000;

    /* Update the power state */
    update_module_power_state(GET_POWER_STATE(soc_pwr_mW, tdp_level_mW));

    if ((soc_pwr_mW > MAX_POWER_THRESHOLD_GUARDBAND(tdp_level_mW)) &&
        (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_STATE_POWER_DOWN) &&
        (get_soc_power_reg()->active_power_management))
    {
        /* Do the power throttling down */
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_POWER_DOWN;
        xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
    }

    /* Switch power throttle state only if we are currently in lower priority throttle
        state and Active Power Management is enabled*/
    if ((soc_pwr_mW < tdp_level_mW) &&
        (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_STATE_POWER_UP) &&
        (get_soc_power_reg()->active_power_management))
    {
        /* Do the power throttling up */
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_POWER_UP;
        xTaskNotify(g_pm_handle, 0, eSetValueWithOverwrite);
    }
#endif
    return 0;
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
    *soc_pwr_10mw = get_soc_power_reg()->soc_pwr_10mW;
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
*       This function gets the voltage value for a given voltage domain.
*
*   INPUTS
*
*       *module_voltage         Pointer to Module voltage struct
*        shire                  The shire specific voltage domain
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_module_voltage(struct module_voltage_t *module_voltage)
{
    int status = STATUS_SUCCESS;
    uint8_t temp;
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    MemShire_VM_sample memshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    PShire_VM_sample pshr_voltage = { { 0, 0, 0 }, { 0, 0, 0 } };

    status = pvt_get_minion_avg_low_high_voltage(&minshire_voltage);
    if (status == STATUS_SUCCESS)
    {
        /* SRAM voltage value is to be stored in l2_cache placeholder as currently no SRAM member is available in module_voltage */
        get_soc_power_reg()->module_voltage.l2_cache =
            PMIC_MILLIVOLT_TO_HEX(minshire_voltage.vdd_sram.current, PMIC_SRAM_VOLTAGE_MULTIPLIER);
        get_soc_power_reg()->module_voltage.minion =
            PMIC_MILLIVOLT_TO_HEX(minshire_voltage.vdd_mnn.current, PMIC_MINION_VOLTAGE_MULTIPLIER);
        get_soc_power_reg()->module_voltage.noc =
            PMIC_MILLIVOLT_TO_HEX(minshire_voltage.vdd_noc.current, PMIC_MINION_VOLTAGE_MULTIPLIER);
        Log_Write(
            LOG_LEVEL_DEBUG,
            "get_module_voltage: L2 Cache/SRAM Voltage: %d \t minion voltage: %d\t noc voltage: %d\r\n",
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
        get_soc_power_reg()->module_voltage.ddr =
            PMIC_MILLIVOLT_TO_HEX(memshire_voltage.vdd_ms.current, PMIC_DDR_VOLTAGE_MULTIPLIER);
        Log_Write(LOG_LEVEL_DEBUG, "get_module_voltage: ddr voltage: %d\r\n",
                  memshire_voltage.vdd_ms.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get memshire voltage\n");
    }

    status = pvt_get_pshire_vm_sample(&pshr_voltage);
    if (status == STATUS_SUCCESS)
    {
        get_soc_power_reg()->module_voltage.pcie =
            PMIC_PCIE_MILLIVOLT_TO_HEX(pshr_voltage.vdd_pshr.current);
        Log_Write(LOG_LEVEL_DEBUG, "get_module_voltage: pcie voltage: %d\r\n",
                  pshr_voltage.vdd_pshr.current);
    }
    else
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: faild to get pshire voltage\n");
    }

    /* For these values we dont have PVT, so will use PMIC */
    pmic_get_voltage(MODULE_MAXION, &temp);
    get_soc_power_reg()->module_voltage.maxion = temp;
    pmic_get_voltage(MODULE_VDDQLP, &temp);
    get_soc_power_reg()->module_voltage.vddqlp = temp;
    pmic_get_voltage(MODULE_VDDQ, &temp);
    get_soc_power_reg()->module_voltage.vddq = temp;
    pmic_get_voltage(MODULE_PCIE_LOGIC, &temp);
    get_soc_power_reg()->module_voltage.pcie_logic = temp;

    if (module_voltage != NULL)
    {
        *module_voltage = get_soc_power_reg()->module_voltage;
    }

    return 0;
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
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_pmb_stats(void)
{
    int status = STATUS_SUCCESS;

    /* obtain PMB values from PMIC */
    status = pmic_get_pmb_stats(&g_soc_power_reg.pmb_stats);

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
    *max_temp = get_soc_power_reg()->max_temp;
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

    get_soc_power_reg()->module_uptime.day = day;      //days
    get_soc_power_reg()->module_uptime.hours = hours;  //hours
    get_soc_power_reg()->module_uptime.mins = minutes; //mins;
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
    *module_uptime = get_soc_power_reg()->module_uptime;
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
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
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
            *residency = get_soc_power_reg()->power_up_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_POWER_DOWN: {
            *residency = get_soc_power_reg()->power_down_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_DOWN: {
            *residency = get_soc_power_reg()->thermal_down_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_POWER_SAFE: {
            *residency = get_soc_power_reg()->power_safe_throttled_states_residency;
            break;
        }
        case POWER_THROTTLE_STATE_THERMAL_SAFE: {
            *residency = get_soc_power_reg()->thermal_safe_throttled_states_residency;
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
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
            *residency = get_soc_power_reg()->power_max_residency;
            break;
        }
        case POWER_STATE_MANAGED_POWER: {
            *residency = get_soc_power_reg()->power_managed_residency;
            break;
        }
        case POWER_STATE_SAFE_POWER: {
            *residency = get_soc_power_reg()->power_safe_residency;
            break;
        }
        case POWER_STATE_LOW_POWER: {
            *residency = get_soc_power_reg()->power_low_residency;
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
    get_soc_power_reg()->event_cb = event_cb;
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

    get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_POWER_IDLE;

    get_soc_power_reg()->active_power_management = ACTIVE_POWER_MANAGEMENT_TURN_OFF;

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
*       uint32_t                  Amount of Power to be reduced by (in mW)
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int reduce_minion_operating_point(int32_t delta_power,
                                         struct trace_event_power_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    int32_t delta_freq = (delta_power * DP_per_Mhz) / 1000;
    int32_t new_freq = Get_Minion_Frequency() - delta_freq;

    /* If we are using modes round frequency to be multiple of MODE_FREQUENCY_STEP_SIZE */
#if USE_FCW_FOR_LVDPLL == 0
    new_freq = ((new_freq + (MODE_FREQUENCY_STEP_SIZE / 2)) / MODE_FREQUENCY_STEP_SIZE) *
               MODE_FREQUENCY_STEP_SIZE;
#endif

    if (0 != Minion_Shire_Update_PLL_Freq((uint16_t)new_freq))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to update minion frequency!\n");
        return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
    }

    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(new_freq);
    if (new_voltage != get_soc_power_reg()->module_voltage.minion)
    {
        //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
    }

    Update_Minion_Frequency_Global_Reg(new_freq);

    power_status->tgt_freq = (uint16_t)new_freq;
    power_status->tgt_voltage = (uint16_t)new_voltage;

    Trace_Power_Status(Trace_Get_SP_CB(), power_status);

    return 0;
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
*       uint32_t                  Amount of Power to be increased by (in mW)
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int increase_minion_operating_point(int32_t delta_power,
                                           struct trace_event_power_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    int32_t delta_freq = (delta_power * DP_per_Mhz) / 1000;
    int32_t new_freq = Get_Minion_Frequency() + delta_freq;

    /* If we are using modes round frequency to be multiple of MODE_FREQUENCY_STEP_SIZE */
#if USE_FCW_FOR_LVDPLL == 0
    new_freq = ((new_freq + (MODE_FREQUENCY_STEP_SIZE / 2)) / MODE_FREQUENCY_STEP_SIZE) *
               MODE_FREQUENCY_STEP_SIZE;
#endif

    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(new_freq);
    if (new_voltage != get_soc_power_reg()->module_voltage.minion)
    {
        //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
    }

    if (0 != Minion_Shire_Update_PLL_Freq((uint16_t)new_freq))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to update minion frequency!\n");
        return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
    }

    Update_Minion_Frequency_Global_Reg(new_freq);

    power_status->tgt_freq = (uint16_t)new_freq;
    power_status->tgt_voltage = (uint16_t)new_voltage;

    Trace_Power_Status(Trace_Get_SP_CB(), power_status);

    return 0;
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

    if (SAFE_STATE_FREQUENCY < Get_Minion_Frequency())
    {
        if (0 != Minion_Shire_Update_PLL_Freq(SAFE_STATE_FREQUENCY))
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to go to safe state!\n");
            return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
        }

        if (new_voltage != get_soc_power_reg()->module_voltage.minion)
        {
            //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
        }
    }
    else if (SAFE_STATE_FREQUENCY > Get_Minion_Frequency())
    {
        if (new_voltage != get_soc_power_reg()->module_voltage.minion)
        {
            //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
        }

        if (0 != Minion_Shire_Update_PLL_Freq(SAFE_STATE_FREQUENCY))
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to go to safe state!\n");
            return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
        }
    }

    Update_Minion_Frequency_Global_Reg(SAFE_STATE_FREQUENCY);

    /* Get the current power */
    if (0 != pmic_read_average_soc_power(&soc_pwr_10mW))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
    }

    if (0 != pvt_get_minion_avg_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
    }

    FILL_POWER_STATUS(power_status, throttle_state, power_state, POWER_10MW_TO_W(soc_pwr_10mW),
                      current_temperature, (uint16_t)SAFE_STATE_FREQUENCY, (uint16_t)new_voltage)

    Trace_Power_Status(Trace_Get_SP_CB(), &power_status);

    return 0;
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
    int32_t delta_power_mW;
    uint8_t throttle_condition_met = 0;
    struct trace_event_power_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == POWER_THROTTLE_STATE_POWER_SAFE)
    {
        go_to_safe_state(get_soc_power_reg()->module_power_state, throttle_state);
    }

    if (0 != pvt_get_minion_avg_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc temperature\r\n");
    }

    if (0 != pmic_read_average_soc_power(&avg_pwr_10mW))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
    }

    /* module_tdp_level is in Watts, converting to miliWatts */
    tdp_level_mW = get_soc_power_reg()->module_tdp_level * 1000;

    while (!throttle_condition_met && (get_soc_power_reg()->power_throttle_state <= throttle_state))
    {
        /* Program the new operating point */
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_POWER_UP: {
                delta_power_mW = ((POWER_10MW_TO_MW(avg_pwr_10mW) * (POWER_SCALE_FACTOR)) / 100);
                FILL_POWER_STATUS(power_status, throttle_state,
                                  get_soc_power_reg()->module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                increase_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_DOWN: {
                delta_power_mW = ((POWER_10MW_TO_MW(avg_pwr_10mW) * (POWER_SCALE_FACTOR)) / 100);
                FILL_POWER_STATUS(power_status, throttle_state,
                                  get_soc_power_reg()->module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                reduce_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case POWER_THROTTLE_STATE_POWER_SAFE: {
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            }
        }

        /* TODO: What should be this delay? */
        vTaskDelay(pdMS_TO_TICKS(DELTA_POWER_UPDATE_PERIOD));

        /* Get the current power */
        if (0 != pmic_read_average_soc_power(&avg_pwr_10mW))
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
        }

        /* Check if throttle condition is met */
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_POWER_UP: {
                if (POWER_10MW_TO_MW(avg_pwr_10mW) > tdp_level_mW)
                {
                    throttle_condition_met = 1;
                }
                break;
            }
            case POWER_THROTTLE_STATE_POWER_DOWN:
            case POWER_THROTTLE_STATE_POWER_SAFE: {
                if (POWER_10MW_TO_MW(avg_pwr_10mW) < MAX_POWER_THRESHOLD_GUARDBAND(tdp_level_mW))
                {
                    throttle_condition_met = 1;
                }
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            }
        }
    }

    end_time = timer_get_ticks_count();

    if (get_soc_power_reg()->power_throttle_state <= throttle_state)
    {
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_POWER_IDLE;
    }

    /* Update the power state */
    if (get_soc_power_reg()->module_power_state != POWER_STATE_MANAGED_POWER)
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
    int32_t delta_power_mW;
    struct trace_event_power_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == POWER_THROTTLE_STATE_THERMAL_SAFE)
    {
        go_to_safe_state(get_soc_power_reg()->module_power_state, throttle_state);
    }

    if (0 != pvt_get_minion_avg_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
    }

    while ((current_temperature > get_soc_power_reg()->temperature_threshold.sw_temperature_c) &&
           (get_soc_power_reg()->power_throttle_state <= throttle_state))
    {
        switch (throttle_state)
        {
            case POWER_THROTTLE_STATE_THERMAL_DOWN: {
                /* Get the current power */
                if (0 != pmic_read_average_soc_power(&avg_pwr_10mW))
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "thermal pwr mgmt svc error: failed to get soc power\r\n");
                }

                delta_power_mW = ((POWER_10MW_TO_MW(avg_pwr_10mW) * (POWER_SCALE_FACTOR)) / 100);

                FILL_POWER_STATUS(power_status, throttle_state,
                                  get_soc_power_reg()->module_power_state,
                                  POWER_10MW_TO_W(avg_pwr_10mW), current_temperature, 0, 0)
                /* Program the new operating point  */
                reduce_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case POWER_THROTTLE_STATE_THERMAL_SAFE: {
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
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

    if (get_soc_power_reg()->power_throttle_state <= throttle_state)
    {
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_THERMAL_IDLE;
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

    while (1)
    {
        while (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_STATE_POWER_UP)
        {
            xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);
        }

        Log_Write(LOG_LEVEL_INFO, "POWER THROTTLING:%d\n",
                  get_soc_power_reg()->power_throttle_state);

        switch (get_soc_power_reg()->power_throttle_state)
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
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
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
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_THERMAL_SAFE;
    }
    else if (PMIC_I2C_INT_CTRL_OV_POWER_GET(int_cause))
    {
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_STATE_POWER_SAFE;
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
    volatile struct soc_power_reg_t const *soc_power_reg = get_soc_power_reg();

    /* Dump power mgmt globals */
    Log_Write(
        LOG_LEVEL_CRITICAL,
        "power_state = %u, tdp_level = %u, temperature = %u c, power = %u W, max_temperature = %u c\n",
        soc_power_reg->module_power_state, soc_power_reg->module_tdp_level,
        soc_power_reg->soc_temperature, POWER_10MW_TO_W(soc_power_reg->soc_pwr_10mW),
        soc_power_reg->max_temp);

    Log_Write(LOG_LEVEL_CRITICAL, "Module uptime (day:hours:mins): %d:%d:%d\n",
              soc_power_reg->module_uptime.day, soc_power_reg->module_uptime.hours,
              soc_power_reg->module_uptime.mins);

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
        "Module Voltages (mV) :  ddr = %u , l2_cache = %u, maxion = %u, minion = %u, pcie = %u, noc = %u, pcie_logic = %u, vddqlp = %u, vddq = %u\n",
        soc_power_reg->module_voltage.ddr, soc_power_reg->module_voltage.l2_cache,
        soc_power_reg->module_voltage.maxion, soc_power_reg->module_voltage.minion,
        soc_power_reg->module_voltage.pcie, soc_power_reg->module_voltage.noc,
        soc_power_reg->module_voltage.pcie_logic, soc_power_reg->module_voltage.vddqlp,
        soc_power_reg->module_voltage.vddq);
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
    pmic_set_voltage(MODULE_MINION, NEIGH_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_MINION, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding Minion -> 475mV (0x%X)\n", voltage);

    /* Setting the L2 cache voltages */
    pmic_set_voltage(MODULE_L2CACHE, SRAM_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_L2CACHE, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding SRAM   -> 720mV(0x%X)\n", voltage);

    /* Setting the NOC voltages */
    pmic_set_voltage(MODULE_NOC, NOC_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_NOC, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding NOC    -> 430mV(0x%X)\n", voltage);

    /* Setting the DDR voltages */
    pmic_set_voltage(MODULE_DDR, DDR_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_DDR, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding DDR    -> 750mV(0x%X)\n", voltage);

    /* Setting the MAXION voltages */
    pmic_set_voltage(MODULE_MAXION, MXN_BOOT_VOLTAGE);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_MAXION, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding MAXION  -> 600mV(0x%X)\n", voltage);
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
    TS_Sample temperature;

    Log_Write(LOG_LEVEL_INFO,
              "SHIRE       \t\tFreq          \t\tVoltage          \t\tTemperature\r\n");
    Log_Write(
        LOG_LEVEL_INFO,
        "---------------------------------------------------------------------------------------------\r\n");

    /* Neigh */
    freq = (uint32_t)Get_Minion_Frequency();
    pvt_get_minion_avg_low_high_temperature(&temperature);
    MinShire_VM_sample minshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    pvt_get_minion_avg_low_high_voltage(&minshire_voltage);
    Log_Write(LOG_LEVEL_INFO,
              "NEIGH       \t\tHPDPLL4 %dMHz\t\t%dmV,[%dmV,%dmV]\t\t%dC,[%dC,%dC]\r\n", freq,
              minshire_voltage.vdd_mnn.current, minshire_voltage.vdd_mnn.low,
              minshire_voltage.vdd_mnn.high, temperature.current, temperature.low,
              temperature.high);

    /* Sram */
    Log_Write(LOG_LEVEL_INFO, "SRAM        \t\tHPDPLL4 %dMHz\t\t%dmV,[%dmV,%dmV]\t\t/\r\n", freq,
              minshire_voltage.vdd_sram.current, minshire_voltage.vdd_sram.low,
              minshire_voltage.vdd_sram.high);

    /* NOC */
    get_pll_frequency(PLL_ID_SP_PLL_2, &freq);
    Log_Write(LOG_LEVEL_INFO, "NOC         \t\tHPDPLL2 %dMHz\t\t%dmV,[%dmV,%dmV]\t\t/\r\n", freq,
              minshire_voltage.vdd_noc.current, minshire_voltage.vdd_noc.low,
              minshire_voltage.vdd_noc.high);

    /* MemShire */
    freq = get_memshire_frequency();
    MemShire_VM_sample memshire_voltage = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    pvt_get_memshire_avg_low_high_voltage(&memshire_voltage);
    Log_Write(LOG_LEVEL_INFO, "MEMSHIRE    \t\tHPDPLLM %dMHz\t\t%dmV,[%dmV,%dmV]\t\t/\r\n", freq,
              memshire_voltage.vdd_ms.current, memshire_voltage.vdd_ms.low,
              memshire_voltage.vdd_ms.high);

    /* IOShire SPIO*/
    get_pll_frequency(PLL_ID_SP_PLL_0, &freq);
    pvt_get_ioshire_ts_sample(&temperature);
    IOShire_VM_sample ioshire_voltage = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
    pvt_get_ioshire_vm_sample(&ioshire_voltage);
    Log_Write(LOG_LEVEL_INFO,
              "IOSHIRE SPIO\t\tHPDPLL0 %dMHz\t\t/                  \t\t%dC,[%dC,%dC]\r\n", freq,
              temperature.current, temperature.low, temperature.high);

    /* IOShire PU*/
    get_pll_frequency(PLL_ID_SP_PLL_1, &freq);
    Log_Write(LOG_LEVEL_INFO, "        PU  \t\tHPDPLL1 %dMHz\t\t%dmV,[%dmV,%dmV]\t\t/\r\n", freq,
              ioshire_voltage.vdd_pu.current, ioshire_voltage.vdd_pu.low,
              ioshire_voltage.vdd_pu.high);

    /* PSHIRE */
    get_pll_frequency(PLL_ID_PSHIRE, &freq);
    PShire_VM_sample pshr_voltage = { { 0, 0, 0 }, { 0, 0, 0 } };
    pvt_get_pshire_vm_sample(&pshr_voltage);
    Log_Write(LOG_LEVEL_INFO, "PSHIRE      \t\tHPDPLLP %dMHz\t\t%dmV,[%dmV,%dmV]\t\t/\r\n", freq,
              pshr_voltage.vdd_pshr.current, pshr_voltage.vdd_pshr.low, pshr_voltage.vdd_pshr.high);

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
    *temp = get_soc_power_reg()->op_stats.minion.temperature.avg;
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
    *temp = get_soc_power_reg()->op_stats.system.temperature.avg;
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
       value is available.
       get_soc_power_reg()->module_voltage.minion */
    *power = get_soc_power_reg()->op_stats.minion.power.avg;
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
       value is available.
       get_soc_power_reg()->module_voltage.noc */

    *power = get_soc_power_reg()->op_stats.noc.power.avg;
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
    value is available.
    get_soc_power_reg()->module_voltage */
    *power = get_soc_power_reg()->op_stats.sram.power.avg;
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
    *power = get_soc_power_reg()->op_stats.system.power.avg;
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
    *stats = get_soc_power_reg()->op_stats;
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

    /* read temperature values from pvt */
    status = pvt_get_minion_avg_temperature(&tmp_val);
    if (status == STATUS_SUCCESS)
    {
        /* initialize temperature valuesin op stats */
        INIT_STAT_VALUE(get_soc_power_reg()->op_stats.minion.temperature, tmp_val);

        /* Update PMB stats */
        status = update_pmb_stats();
        if (status == STATUS_SUCCESS)
        {
            /* Initialize stats min, max andd avg values */
            INIT_STAT_VALUE(get_soc_power_reg()->op_stats.minion.power,
                            get_soc_power_reg()->pmb_stats.minion.w_out.average)
            INIT_STAT_VALUE(get_soc_power_reg()->op_stats.sram.power,
                            get_soc_power_reg()->pmb_stats.sram.w_out.average)
            INIT_STAT_VALUE(get_soc_power_reg()->op_stats.noc.power,
                            get_soc_power_reg()->pmb_stats.noc.w_out.average)

            /* Read card average power */
            status = pmic_read_average_soc_power(&soc_pwr_10mW);
        }

        /* Updater card average power */
        if (status == STATUS_SUCCESS)
        {
            /* initialize op stats with average power in mW */
            INIT_STAT_VALUE(get_soc_power_reg()->op_stats.system.power, soc_pwr_10mW)
        }
    }

    return status;
}
