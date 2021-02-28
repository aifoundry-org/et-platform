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
/*! \file thermal_pwr_mgmt.h
    \brief A C header that defines the Thermal and power manangement service's
    public interfaces. These interfaces provide services using which
    the host can query device for thermal and power related details.
*/
/***********************************************************************/
#include "dm.h"
#include "dm_service.h"
#include "dm_task.h"

/*! \fn void update_module_max_throttle_time_gbl(void)
    \brief Interface to read the module max throttle time from hw and update the global variable
    \param none
    \returns none
*/
void update_module_max_throttle_time_gbl(void);

/*! \fn uint64_t get_module_max_throttle_time_gbl(void)
    \brief Interface to get the module max throttle time from the global variable
    \param none
    \returns uint64_t 
*/
uint64_t get_module_max_throttle_time_gbl(void);

/*! \fn void update_module_max_temp_gbl(uint8_t)
    \brief Interface to update module max temperature in global variable
    \param uint8_t max temperature value
    \returns none 
*/
void update_module_max_temp_gbl(uint8_t);

/*! \fn uint8_t get_module_max_temperature_gbl(void)
    \brief Interface to get module max temperature from global variable
    \param none
    \returns uint8_t
*/
uint8_t get_module_max_temperature_gbl(void);