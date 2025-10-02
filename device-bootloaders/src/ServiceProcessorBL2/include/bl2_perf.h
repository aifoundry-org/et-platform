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
