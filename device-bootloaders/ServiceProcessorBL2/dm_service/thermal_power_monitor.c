/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------

************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements the following Power Management services.
*
*   FUNCTIONS
*
*       Host Requested Services functions:
*       
*       - thermal_power_monitoring_process
*       - pwr_svc_{get|set}_module_power_state
*       - pwr_svc_{get|set}_module_tdp_level
*       - pwr_svc_{get|set}_module_temp_thresholds
*       - pwr_svc_{get|set}_module_current_temperature
*       - pwr_svc_get_module_residency_throttle_states
*       - pwr_svc_get_module_power
*       - pwr_svc_get_module_uptime
*       - pwr_svc_get_module_voltage
*       - pwr_svc_get_module_max_temperature
*
***********************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "dm.h"
#include "bl2_pmic_controller.h"

#include "bl2_thermal_power_monitor.h"

/************************************************************************************
This struct is temporary placed here - as it will move to actual Power Management
function which will initialize the Global Register state
************************************************************************************/
struct soc_power_reg_t *g_soc_power_reg __attribute__((section(".data")));


static uint8_t cqueue_push(char * buf, uint32_t size) 
{
     printf("Pointer to buf %s with payload size: %d\n", buf, size); 
     return 0;
}

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
static void pwr_svc_get_module_power_state(uint64_t req_start_time)
{
    struct power_state_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.pwr_state);

    dm_rsp.pwr_state = g_soc_power_reg->module_power_state;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_power_state: Cqueue push error !\n");
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
static void pwr_svc_set_module_power_state(uint64_t req_start_time, power_state_t state)
{
    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    g_soc_power_reg->module_power_state = state;

    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

    if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_set_module_power_state: Cqueue push error !\n");
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
static void pwr_svc_get_module_power(uint64_t req_start_time)
{
    struct module_power_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.watts);
    
    g_soc_power_reg->soc_power = pmic_read_soc_power(); 
    dm_rsp.watts = g_soc_power_reg->soc_power;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

    if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_power: Cqueue push error!\n");
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
*       shire             The shire specific voltage domain
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void pwr_svc_get_module_voltage(uint64_t req_start_time, module_t shire)
{
    struct module_voltage_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.volts);

    g_soc_power_reg->voltage[shire] = pmic_get_voltage(shire);     
    dm_rsp.volts = g_soc_power_reg->voltage[shire];

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_voltage: Cqueue push error!\n");
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
*       This function return the Thermal Design Power(TDP) of the System. This
*       refers to power consumption under the maximum theoretical load level.
*       *******************************************
*       | Form Factor | Level | Watts | Mode      |
*       *******************************************
*       |             |   1   | 15 W  | Efficient |
*       | Dual M2     |   2   | 20 W  | Normal    |
*       |             |   3   | 25 W  | Max Perf  |
*       *******************************************
*       | PCI         |   4   | 75 W  | Normal    |
*       *******************************************
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
static void pwr_svc_get_module_tdp_level(uint64_t req_start_time)
{
    struct tdp_level_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.tdp_level);

    dm_rsp.tdp_level = g_soc_power_reg->module_tdp_level;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_tdp_level: Cqueue push error!\n");
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
static void pwr_svc_set_module_tdp_level(uint64_t req_start_time, tdp_level_t tdp)
{
    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    // TODO implement response handler for PMIC Read
    pmic_set_tdp_threshold(tdp);
    g_soc_power_reg->module_tdp_level = tdp;

    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

    if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_set_module_power_state: Cqueue push error !\n");
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
static void pwr_svc_get_module_temp_thresholds(uint64_t req_start_time)
{
    struct temperature_threshold_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.lo_temperature_c) + sizeof(dm_rsp.hi_temperature_c);
    dm_rsp.lo_temperature_c = g_soc_power_reg->module_temp_lo_threshold;
    dm_rsp.hi_temperature_c = g_soc_power_reg->module_temp_hi_threshold;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp));  

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_temp_thresholds: Cqueue push error !\n");
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
static void pwr_svc_set_module_temp_thresholds(uint64_t req_start_time, uint8_t hi_threshold, uint8_t lo_threshold)
{
    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    pmic_set_temperature_threshold(L0, lo_threshold);
    pmic_set_temperature_threshold(HI, hi_threshold);
    g_soc_power_reg->module_temp_lo_threshold = lo_threshold;
    g_soc_power_reg->module_temp_hi_threshold = hi_threshold;
   
    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp));  
     
    if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_set_module_temp_thresholds: Cqueue push error !\n");
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
static void pwr_svc_get_module_current_temperature(uint64_t req_start_time)
{
    struct current_temperature_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.temperature_c);

    g_soc_power_reg->soc_temperature = pmic_get_temperature();
    dm_rsp.temperature_c = g_soc_power_reg->soc_temperature;
    
    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 
 
   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_current_temperature: Cqueue push error!\n");
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
*       throttles state i.e. non-full power mode from Device Reset
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
static void pwr_svc_get_module_residency_throttle_states(uint64_t req_start_time)
{
    struct throttle_time_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.time_usec);

    dm_rsp.time_usec = g_soc_power_reg->throttled_states_residency;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp));

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_residency_throttle_states: Cqueue push error!\n");
   }
}

/************************************************************************
*
*   FUNCTION
*
*       pwr_svc_get_module_max_temperature
*  
*   DESCRIPTION
*
*       This function returns the historical_extreme Maximum Device temperature
*       as sample periodically from the PMIC
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
static void pwr_svc_get_module_max_temperature(uint64_t req_start_time)
{
    struct max_temperature_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.max_temperature_c);

    dm_rsp.max_temperature_c = g_soc_power_reg->max_temp;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_max_temperature: Cqueue push error!\n");
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
static void pwr_svc_get_module_uptime(uint64_t req_start_time)
{
    struct module_uptime_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.time_day) + sizeof(dm_rsp.time_hours) + sizeof(dm_rsp.time_seconds);

    dm_rsp.time_day = g_soc_power_reg->module_uptime[0];
    dm_rsp.time_hours = g_soc_power_reg->module_uptime[1];
    dm_rsp.time_seconds = g_soc_power_reg->module_uptime[2];

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp));

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("pwr_svc_get_module_uptime: Cqueue push error!\n");
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
*       cmd_id      Unique enum representing specific command   
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void thermal_power_monitoring_process(uint32_t cmd_id)
{
    uint64_t req_start_time;
    
    req_start_time = timer_get_ticks_count();

     switch (cmd_id) {
      case GET_MODULE_POWER_STATE: {
         pwr_svc_get_module_power_state(req_start_time);
      } break;
      case SET_MODULE_POWER_STATE: {
         pwr_svc_set_module_power_state(req_start_time, REDUCED);
      } break;
      case GET_MODULE_STATIC_TDP_LEVEL: {
         pwr_svc_get_module_tdp_level(req_start_time);
      } break;
      case SET_MODULE_STATIC_TDP_LEVEL: {
         pwr_svc_set_module_tdp_level(req_start_time, LEVEL_1);
      } break;
      case GET_MODULE_TEMPERATURE_THRESHOLDS: {
         pwr_svc_get_module_temp_thresholds(req_start_time);
      } break;
      case SET_MODULE_TEMPERATURE_THRESHOLDS: {
         pwr_svc_set_module_temp_thresholds(req_start_time, 100, 80);
      } break;
      case GET_MODULE_CURRENT_TEMPERATURE: {
         pwr_svc_get_module_current_temperature(req_start_time);
      } break;
      case GET_MODULE_RESIDENCY_THROTTLE_STATES: {
         pwr_svc_get_module_residency_throttle_states(req_start_time);
      } break;
      case GET_MODULE_POWER: {
         pwr_svc_get_module_power(req_start_time);
      } break;
      case GET_MODULE_VOLTAGE: {
         pwr_svc_get_module_voltage(req_start_time, MINION);
      } break;
      case GET_MODULE_UPTIME: {
         pwr_svc_get_module_uptime(req_start_time);
      } break;
      case GET_MODULE_MAX_TEMPERATURE: {
         pwr_svc_get_module_max_temperature(req_start_time);
      } break;
      }
	 
}
