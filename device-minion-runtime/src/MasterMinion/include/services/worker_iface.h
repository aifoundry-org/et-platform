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
*       Header/Interface description for public interfaces that provide
*       interface to workers
*
***********************************************************************/
#ifndef WORKER_IFACE_DEFS_H
#define WORKER_IFACE_DEFS_H

#include "common_defs.h"

enum WORKER_IFACE_TYPE
{
    TO_KW_FIFO,
    TO_DMAW_FIFO,
    TO_CQW_FIFO
};

/*! \fn int8_t Worker_Iface_Init(uint8_t interface_type)
    \brief Initializes the interface specified by the caller
    \param interface_type
    \return status
*/
int8_t Worker_Iface_Init(uint8_t interface_type);

/*! \fn int8_t Worker_Iface_Push_Cmd(void* p_cmd, uint32_t cmd_size)
    \brief Push command to interface specied by caller
    \param [in] interface_type: Caller to specify a supported interface type
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size
    \returns [out] Status
*/
int8_t Worker_Iface_Push_Cmd(uint8_t interface_type, void* p_cmd, 
        uint32_t cmd_size);

/*! \fn int8_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff)
    \brief Push command from interface specified by caller
    \param [in] interface_type: Caller to specify a supported interface type
    \param [in] rx_buff: Buffer to copy retrieved command
    \returns [out] Command size
*/
uint32_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff);

/*! \fn int8_t Worker_Iface_Deinit(uint8_t interface_type)
    \brief Deinitializes the interface specified by the caller
    \param interface_type
    \return status
*/
int8_t Worker_Iface_Deinit(uint8_t interface_type);

#endif /* WORKER_IFACE_DEFS_H */