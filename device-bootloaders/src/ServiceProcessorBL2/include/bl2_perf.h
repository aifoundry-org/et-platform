/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_perf.h
    \brief A C header that defines the Performance service's
    public interfaces. These interfaces provide services using which
    the host can query about device performance metrics.
*/
/***********************************************************************/
#ifndef __BL2_PERF_H__
#define __BL2_PERF_H__

#include <stdint.h>
#include "sp_host_iface.h"
#include "perf_mgmt.h"

/*! \fn void process_performance_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
    \brief Interface to process the performance request command
    by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \param buffer Pointer to command buffer
    \returns none
*/
void process_performance_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

#endif
