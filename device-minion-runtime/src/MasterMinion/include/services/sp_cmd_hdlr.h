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
/***********************************************************************/
/*! \file sp_cmd_hdlr.h
    \brief A C header that defines the Service Processor command 
    handler responsible for handling all commands from Service Processor
    Interface
*/
/***********************************************************************/
#ifndef SP_CMD_HDLR_H
#define SP_CMD_HDLR_H

#include <common_defs.h>

/*! \fn SP_Command_Handler(void* command_buffer, uint16_t command_size)
    \brief Interface to handle host side commands
    \param command_buffer: pointer to command buffer
    \param command_size: command size
    \return int8_t: Status indicating success or negative error
*/
int8_t SP_Command_Handler(void* command_buffer, uint16_t command_size);

#endif