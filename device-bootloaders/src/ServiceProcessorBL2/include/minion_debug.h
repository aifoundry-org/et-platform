/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
#include "minion_run_control.h"
#include "minion_state_inspection.h"
#include "log.h"

#define NO_OF_GPR_REGS 32

#define MEM_READ64(addr)        *(volatile uint64_t *)((uint64_t)addr)
#define MEM_WRITE64(addr, data) *(volatile uint64_t *)((uint64_t)(addr)) = (uint64_t)(data)

/*! \fn void minion_debug_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
    \brief Function takes command ID as input from Host, and accordingly calls the respective minion debug
*       run control/state inspection APIs.
    \param tag_id Tag ID
    \param msg_id Message ID Type of the command we have received
    \returns none
*/
void minion_debug_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer);

#endif /* MINION_DEBUG_H */