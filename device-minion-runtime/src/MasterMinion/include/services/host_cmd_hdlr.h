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
/*! \file host_cmd_hdlr.h
    \brief A C header that defines the Host Command Handler component's
    public interfaces. This interface privides services to handle all
    commands received from the Host over PCIe. 
*/
/***********************************************************************/
#ifndef HOST_CMD_HDLR_H
#define HOST_CMD_HDLR_H

#include <common_defs.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>

/*! \def calculate_latency 
    \brief A macro to calculate latency. Uses PMC Counter 3 to get current cycle 
    minus start_cycle(argument)
*/
#define calculate_latency(x) (uint32_t)(read_neigh_pmc(PMU_MHPMEVENT3) - x)

/*! \fn int8_t Host_Command_Handler(void* command_buffer)
    \brief Interface to handle host side commands
    \param command_buffer pointer to command buffer
    \param start_cycle cycle count to measure wait latency
    \return status success or negative error code
*/
int8_t Host_Command_Handler(void* command_buffer, uint64_t start_cycle);

#endif /* HOST_CMD_HDLR_H */
