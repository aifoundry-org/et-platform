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
/*! \file bl2_historical_extreme.h
    \brief A C header that defines the Historical extreme value service's
    public interfaces. These interfaces provide services using which
    the host can query device for historical extreme values for certain
    device parameters.
*/
/***********************************************************************/

#ifndef __BL2_HISTORICAL_EXTREME_H__
#define __BL2_HISTORICAL_EXTREME_H__

#include <stdint.h>
#include "dm.h"
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"
#include "sp_host_iface.h"
#include "bl2_ddr_init.h"
#include "dm_event_control.h"

/*! \fn void historical_extreme_value_request(tag_id_t tag_id, msg_id_t msg_id)
    \brief Interface to process the firmware service command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \returns none
*/
void historical_extreme_value_request(tag_id_t tag_id, msg_id_t msg_id);

#endif
