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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface to access host command handler
*
***********************************************************************/
#ifndef HOST_CMD_HDLR_H
#define HOST_CMD_HDLR_H

#include <common_defs.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>

/*! \fn Host_Command_Handler(void* command_buffer, uint16_t command_size)
    \brief Interface to handle host side commands.
    \param [in] command_buffer: pointer to command buffer
    \param [in] command_size: command size
    \param [out] int8_t: command handling return status
*/
int8_t Host_Command_Handler(void* command_buffer);

#endif