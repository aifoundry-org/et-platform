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
/*! \file bl2_link_mgmt.h
    \brief A C header that defines the Link Management service's
    public interfaces. These interfaces provide services using which
    the host can perform link management related operations.
*/
/***********************************************************************/

#ifndef __BL2_LINK_MGMT_H__
#define __BL2_LINK_MGMT_H__

#include <stdint.h>
#include "dm.h"
#include "perf_mgmt.h"
#include "sp_host_iface.h"
#include "bl2_reset.h"
#include "mem_controller.h"
#include "bl2_cache_control.h"
#include "pcie_configuration.h"


/*! \fn void link_mgmt_process_request(tag_id_t tag_id, msg_id_t msg_id)
    \brief Interface to process the link management command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \returns none
*/
void link_mgmt_process_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

#endif
