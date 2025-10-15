/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
