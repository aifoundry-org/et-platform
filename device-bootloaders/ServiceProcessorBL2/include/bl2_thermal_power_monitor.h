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
/*! \file bl2_thermal_power_monitor.h
    \brief A C header that defines the Thermal and power service's
    public interfaces. These interfaces provide services using which
    the host runtime can configure/query about device thermal/power metrics.
*/
/***********************************************************************/
#ifndef __BL2_THERMAL_POWER_MONITOR_H__
#define __BL2_THERMAL_POWER_MONITOR_H__

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "thermal_pwr_mgmt.h"
#include "sp_host_iface.h"

/*! \fn void thermal_power_monitoring_process(tag_id_t tag_id, msg_id_t msg_id)
    \brief Interface to process the performance request command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \returns none
*/
void thermal_power_monitoring_process(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

#endif
