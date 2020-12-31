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
/*! \file worker_iface.h
    \brief A C header that defines the Worker Interface component's 
    public interfaces
*/
/***********************************************************************/
#ifndef WORKER_IFACE_DEFS_H
#define WORKER_IFACE_DEFS_H

#include "common_defs.h"

/**
 * @brief Enum of supported worker interface destinations.
 */
enum WORKER_IFACE_TYPE
{
    TO_KW_FIFO,
    TO_DMAW_FIFO,
    TO_CQW_FIFO
};

/*! \fn int8_t Worker_Iface_Init(uint8_t interface_type)
    \brief Initializes the interface specified by the caller
    \param interface_type KW or CQW or DMAW
    \return Status indicating success or negative error code
*/
int8_t Worker_Iface_Init(uint8_t interface_type);

/*! \fn int8_t Worker_Iface_Push_Cmd(void* p_cmd, uint32_t cmd_size)
    \brief Push command to interface specied by caller
    \param interface_type: Caller to specify a supported interface type
    \param p_cmd Pointer to command rx buffer
    \param cmd_size Command size
    \returns Status indicating success or native error code
*/
int8_t Worker_Iface_Push_Cmd(uint8_t interface_type, void* p_cmd, 
        uint32_t cmd_size);

/*! \fn int8_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff)
    \brief Push command from interface specified by caller
    \param interface_type: Caller to specify a supported interface type
    \param rx_buff: Buffer to copy retrieved command
    \returns Command size
*/
uint32_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff);

/*! \fn int8_t Worker_Iface_Deinit(uint8_t interface_type)
    \brief Deinitializes the interface specified by the caller
    \param interface_type KQ, CQW, or DMAW
    \return Status indicating success or negative error
*/
int8_t Worker_Iface_Deinit(uint8_t interface_type);

#endif /* WORKER_IFACE_DEFS_H */