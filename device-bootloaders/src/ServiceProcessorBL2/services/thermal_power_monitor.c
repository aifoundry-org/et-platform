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
/*! \file thermal_power_monitor.c
    \brief A C module that implements the Power Management services

    Public interfaces:
        thermal_power_monitoring_process
*/
/***********************************************************************/

#include "bl2_thermal_power_monitor.h"

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_power_state
*
*   DESCRIPTION
*
*       This function returns the Power state of the System functioning at. The
*       table depicts the different Power states.
*       *******************************
*       | Type    | Level | Perf      |
*       *******************************
*       | Full    | D0    | Max       |
*       | Reduced | D1,D2 | Minion    |
*       |         |       | Managed   |
*       | Lowest  | D3    | Ref Clock |
*       *******************************
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_power_state(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_power_state_rsp_t dm_rsp;
    power_state_e power_state;
    int32_t status;

    status = get_module_power_state(&power_state);
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: get_module_power_state()\r\n");
    }
    else
    {
        dm_rsp.pwr_state = power_state;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_POWER_STATE,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_power_state_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_power_state: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_set_module_power_state
*
*   DESCRIPTION
*
*       This function set the Power state of the System to function at. The
*       table depicts the different Power states.
*       *******************************
*       | Type    | Level | Perf      |
*       *******************************
*       | Full    | D0    |  Max      |
*       | Reduced | D1    | Managed   |
*       | Lowest  | D3    | Ref Clock |
*       *******************************
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
// TODO: Remove this function and posibility to update power state from outside,
//        it is handled internaly
static void pwr_svc_set_module_power_state(uint16_t tag, uint64_t req_start_time,
                                           power_state_e state)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    status = update_module_power_state(state);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: update_module_power_state()\r\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_MODULE_POWER_STATE,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_set_module_power_state: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_power
*
*   DESCRIPTION
*
*       This function returns the current Power consumed by the whole Device.
*       The value is in Watts
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_power(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_module_power_rsp_t dm_rsp;
    uint8_t soc_power;
    int32_t status;

    status = get_module_soc_power(&soc_power);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: get_module_soc_power()\r\n");
    }
    else
    {
        dm_rsp.module_power.power = soc_power;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_POWER, timer_get_ticks_count() - req_start_time,
                    status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_module_power_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_power: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_voltage
*
*   DESCRIPTION
*
*       This function returns the value of voltage for a given voltage domain.
*       Note there are many voltage domains in the device, hence this will take
*       as an argument the domain of interest
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_voltage(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_module_voltage_rsp_t dm_rsp;
    struct module_voltage_t module_voltage;
    int32_t status;

    status = get_module_voltage(&module_voltage);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: get_module_voltage()\r\n");
    }
    else
    {
        dm_rsp.module_voltage = module_voltage;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_VOLTAGE,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_module_voltage_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_voltage: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_tdp_level
*
*   DESCRIPTION
*
*       This function return the Thermal Design Power(TDP) of the System:
*        - SW_threshold - takes action by itself to reduce the power consumption
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_tdp_level(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_tdp_level_rsp_t dm_rsp;
    uint8_t tdp;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Thermal & pwr mgmt request: %s\n", __func__);

    status = get_module_tdp_level(&tdp);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: get_module_tdp_level()\r\n");
    }
    else
    {
        dm_rsp.tdp_level = tdp;
    }

    Log_Write(LOG_LEVEL_INFO, "Thermal & pwr mgmt response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_STATIC_TDP_LEVEL,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_tdp_level_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_tdp_level: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_set_module_tdp_level
*
*   DESCRIPTION
*
*       This function set the Thermal Design Power(TDP) - power consumption under
*       the maximum theoretical load level. The TDP is set in PMIC to take action
*       by itself to reduce the power consumption
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_set_module_tdp_level(uint16_t tag, uint64_t req_start_time, uint8_t tdp)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    status = update_module_tdp_level(tdp);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt error: update_module_tdp_level()\r\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_MODULE_STATIC_TDP_LEVEL,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_set_module_power_state: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_temp_thresholds
*
*   DESCRIPTION
*
*       This function returns the temperature thresholds which is set within the PMIC:
*        - Inform_threshold -  triggers an interrupt back to SP to take action
*        - Warning_threshold - takes action by itself to reduce the power consumption
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_temp_thresholds(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_temperature_threshold_rsp_t dm_rsp;
    struct temperature_threshold_t temperature_threshold;
    int32_t status;

    status = get_module_temperature_threshold(&temperature_threshold);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  " thermal pwr mgmt error: get_module_temperature_threshold()\r\n");
    }
    else
    {
        dm_rsp.temperature_threshold = temperature_threshold;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_temperature_threshold_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_temp_thresholds: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_set_module_temp_thresholds
*
*   DESCRIPTION
*
*       This function sets the temperature thresholds within the PMIC to:
*        - Inform_threshold -  trigger an interrupt back to SP to take action
*        - Warning_threshold - take action by itself to reduce the power consumption
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_set_module_temp_thresholds(uint16_t tag, uint64_t req_start_time,
                                               uint8_t sw_threshold)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Thermal & pwr mgmt request: %s\n", __func__);

    status = update_module_temperature_threshold(sw_threshold);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "thermal pwr mgmt error: update_module_temperature_threshold()\r\n");
    }

    Log_Write(LOG_LEVEL_INFO, "Thermal & pwr mgmt response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_set_module_temp_thresholds: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_current_temperature
*
*   DESCRIPTION
*
*       This function returns the current Device temperature
*       as sampled from the PMIC
*       Note the value doesnt hold over a consequtitve Device Reset
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_current_temperature(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_current_temperature_rsp_t dm_rsp;
    uint8_t soc_temperature;
    int32_t status;

    status = get_module_current_temperature(&soc_temperature);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt error: get_module_current_temperature()\r\n");
    }
    else
    {
        dm_rsp.current_temperature.temperature_c = soc_temperature;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_CURRENT_TEMPERATURE,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_current_temperature_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_current_temperature: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_residency_throttle_states
*
*   DESCRIPTION
*
*       This function returns the total time the device has been resident in the
*       throttles state, cumulative, average, maximum and minimum
*       Note the value doesnt hold over a consequtitve Device Reset
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       throttle_state    Throttle state for which residency will be fetched
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_residency_throttle_states(uint16_t tag, uint64_t req_start_time, 
                                                            power_throttle_state_e throttle_state)
{
    struct device_mgmt_throttle_residency_rsp_t dm_rsp;
    struct residency_t residency;
    int32_t status;

    Log_Write(LOG_LEVEL_ERROR, "MICHA: throttle state: %d\r\n", throttle_state);

    status = get_throttle_residency(throttle_state, &residency);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt error: get_throttle_time()\r\n");
    }
    else
    {
        dm_rsp.throttle_residency = residency;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, 
                                    sizeof(struct device_mgmt_throttle_residency_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "pwr_svc_get_module_residency_throttle_states: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_residency_power_states
*
*   DESCRIPTION
*
*       This function returns the total time the device has been resident in the
*       power state, cumulative, average, maximum and minimum
*       Note the value doesnt hold over a consequtitve Device Reset
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       power_state       Power state for which residency will be fetched
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_residency_power_states(uint16_t tag, uint64_t req_start_time, 
                                                            power_state_e power_state)
{
    struct device_mgmt_power_residency_rsp_t dm_rsp;
    struct residency_t residency;
    int32_t status;

    status = get_power_residency(power_state, &residency);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt error: get_throttle_time()\r\n");
    }
    else
    {
        dm_rsp.power_residency = residency;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES,
                    timer_get_ticks_count() - req_start_time, status)

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, 
                                    sizeof(struct device_mgmt_power_residency_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "pwr_svc_get_module_residency_power_states: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_uptime
*
*   DESCRIPTION
*
*       This function returns the cumulative time starting from Device Reset.
*       Note the value doesnt hold over a conseqeutitve Device Reset
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_uptime(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_module_uptime_rsp_t dm_rsp;
    struct module_uptime_t module_uptime;
    int32_t status;

    status = get_module_uptime(&module_uptime);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt error: get_module_uptime()\r\n");
    }
    else
    {
        dm_rsp.module_uptime = module_uptime;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_UPTIME, timer_get_ticks_count() - req_start_time,
                    status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_module_uptime_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "pwr_svc_get_module_uptime: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       thermal_power_monitoring_process
*
*   DESCRIPTION
*
*       This function takes as input the command ID from Host,
*       and accordingly either calls the respective power state
*       functions
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void thermal_power_monitoring_process(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    uint64_t req_start_time;
    req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_GET_MODULE_POWER_STATE: {
            pwr_svc_get_module_power_state(tag_id, req_start_time);
            break;
        }
        case DM_CMD_SET_MODULE_POWER_STATE: {
            struct device_mgmt_power_state_cmd_t *power_state_cmd =
                (struct device_mgmt_power_state_cmd_t *)buffer;
            pwr_svc_set_module_power_state(tag_id, req_start_time, power_state_cmd->pwr_state);
            break;
        }
        case DM_CMD_GET_MODULE_STATIC_TDP_LEVEL: {
            pwr_svc_get_module_tdp_level(tag_id, req_start_time);
            break;
        }
        case DM_CMD_SET_MODULE_STATIC_TDP_LEVEL: {
            struct device_mgmt_tdp_level_cmd_t *tdp_level_cmd =
                (struct device_mgmt_tdp_level_cmd_t *)buffer;
            pwr_svc_set_module_tdp_level(tag_id, req_start_time, tdp_level_cmd->tdp_level);
            break;
        }
        case DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS: {
            pwr_svc_get_module_temp_thresholds(tag_id, req_start_time);
            break;
        }
        case DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS: {
            struct device_mgmt_temperature_threshold_cmd_t *threshold_cmd = buffer;
            pwr_svc_set_module_temp_thresholds(
                tag_id, req_start_time, threshold_cmd->temperature_threshold.sw_temperature_c);
            break;
        }
        case DM_CMD_GET_MODULE_CURRENT_TEMPERATURE: {
            pwr_svc_get_module_current_temperature(tag_id, req_start_time);
            break;
        }
        case DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES: {
            const struct device_mgmt_throttle_residency_cmd_t *throttle_residency_cmd =
                (struct device_mgmt_throttle_residency_cmd_t *)buffer;
            pwr_svc_get_module_residency_throttle_states(
                tag_id, req_start_time, throttle_residency_cmd->pwr_throttle_state);
            break;
        }
        case DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES: {
           const struct device_mgmt_power_residency_cmd_t *power_residency_cmd =
                (struct device_mgmt_power_residency_cmd_t *)buffer;
            pwr_svc_get_module_residency_power_states(
                tag_id, req_start_time, power_residency_cmd->pwr_state);
            break;
        }
        case DM_CMD_GET_MODULE_POWER: {
            pwr_svc_get_module_power(tag_id, req_start_time);
            break;
        }
        case DM_CMD_GET_MODULE_VOLTAGE: {
            pwr_svc_get_module_voltage(tag_id, req_start_time);
            break;
        }
        case DM_CMD_GET_MODULE_UPTIME: {
            pwr_svc_get_module_uptime(tag_id, req_start_time);
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unsupported message id!\n");
            break;
        }
    }
}
