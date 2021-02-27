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
        
*/
/***********************************************************************/
#include "thermal_pwr_mgmt.h"

void update_module_max_throttle_time(void) {
     // TODO : Set it to valid value 
     // https://esperantotech.atlassian.net/browse/SW-6559
     get_soc_power_reg()->max_throttled_states_residency = 1000;

}

uint64_t get_module_max_throttle_time_gbl(void) {
     return get_soc_power_reg()->max_throttled_states_residency;
}

void update_gbl_module_max_temp(uint8_t max_temp) {
     get_soc_power_reg()->max_temp = max_temp;
}

uint8_t get_module_max_temperature_gbl(void) {
     return get_soc_power_reg()->max_temp;
}