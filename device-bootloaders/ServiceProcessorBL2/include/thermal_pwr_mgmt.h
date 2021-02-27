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

void update_module_max_throttle_time(void);
uint64_t get_module_max_throttle_time_gbl(void);
void update_gbl_module_max_temp(uint8_t);
uint8_t get_module_max_temperature_gbl(void);