/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file minion_debug.h
    \brief A C header that defines the wrapper for handling minion debug
    interface commands.
*/
/***********************************************************************/
#ifndef MINION_DEBUG_H
#define MINION_DEBUG_H

#include "dm.h"
#include "sp_host_iface.h"
#include "xml2devaddr_map.h"
#include "minion_run_control.h"
#include "minion_state_inspection.h"
#include "cache_flush_ops.h"
#include "log.h"

/*! \fn int32_t minion_debug_init(void)
    \brief Function to configure MDI DDR region by requesting DDr size from SP.
    \returns status of function call
*/
int32_t minion_debug_init(void);

/*! \fn void minion_debug_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
    \brief Function takes command ID as input from Host, and accordingly calls the respective minion debug
*       run control/state inspection APIs.
    \param tag_id Tag ID
    \param msg_id Message ID Type of the command we have received
    \returns none
*/
void minion_debug_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

typedef struct
{
    uint64_t start_addr;
    uint64_t size;
} mem_region;

#endif /* MINION_DEBUG_H */
