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
/*! \file bl2_asset_trk.h
    \brief A C header that defines the Asset tracking service's
    public interfaces. These interfaces provide services using which
    the host can query device for asset details.
*/
/***********************************************************************/
#ifndef ASSET_TRACKING_SERVICE_H
#define ASSET_TRACKING_SERVICE_H

#include "bl2_asset_trk_mgmt.h"
#include "sp_host_iface.h"

/*! \fn void asset_tracking_process_request(tag_id_t tag_id, msg_id_t msg_id, const void *buffer)
    \brief Interface to process the asset tracking command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id Message ID Type of the command we have received
    \param buffer Pointer to Command Payload buffer
    \returns none
*/
void asset_tracking_process_request(tag_id_t tag_id, msg_id_t msg_id, const void *buffer);

#endif
