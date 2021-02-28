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
/*! \file pwr_mgmt.c
    \brief A C module that abstracts the Power Management services

    Public interfaces:
    update_module_max_throttle_time_gbl
    get_module_max_throttle_time_gbl
    update_module_max_temp_gbl
    get_module_max_temperature_gbl
*/
/***********************************************************************/
#include "thermal_pwr_mgmt.h"

/************************************************************************
*
*   FUNCTION
*
*       get_dram_bw
*
*   DESCRIPTION
*
*       This function gets module's max throttle time details and 
*       updates the global variable
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
void update_module_max_throttle_time_gbl(void) {
     // TODO : Set it to valid value 
     // https://esperantotech.atlassian.net/browse/SW-6559
     get_soc_power_reg()->max_throttled_states_residency = 1000;

}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_throttle_time_gbl
*
*   DESCRIPTION
*
*       This function gets Max throttle time from the global variable.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t        Module's max throttle time
*
***********************************************************************/
uint64_t get_module_max_throttle_time_gbl(void) {
     return get_soc_power_reg()->max_throttled_states_residency;
}

/************************************************************************
*
*   FUNCTION
*
*       update_gbl_module_max_temp
*
*   DESCRIPTION
*
*       This function updates the max temperature in global variable.
*
*   INPUTS
*
*       uint8_t    max temperature value
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void update_module_max_temp_gbl(uint8_t max_temp) {
     get_soc_power_reg()->max_temp = max_temp;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_temperature_gbl
*
*   DESCRIPTION
*
*       This function gets module's max temperature from the global variable
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint8_t     max temperature value
*
***********************************************************************/
uint8_t get_module_max_temperature_gbl(void) {
     return get_soc_power_reg()->max_temp;
}