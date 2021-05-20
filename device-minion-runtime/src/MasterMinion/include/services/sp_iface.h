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
/*! \file sp_iface.h
    \brief A C header that defines the Service Processor public
    interfaces.
*/
/***********************************************************************/
#ifndef SP_IFACE_DEFS_H
#define SP_IFACE_DEFS_H

#include "common_defs.h"
#include "sp_mm_iface.h" /* header from shared/helper lib */

/*! \fn int8_t SP_Iface_Init(void)
    \brief Initialize Mm interface to Service Processor (SP)
    \return Status indicating success or negative error
*/
#define SP_Iface_Init()   SP_MM_Iface_Init()

/*! \fn int8_t SP_Iface_Push_Cmd_To_MM2SP_SQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Master Minion (MM) to Service Processor (SP)
    Submission Queue(SQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define SP_Iface_Push_Cmd_To_MM2SP_SQ(p_cmd, cmd_size)   \
    SP_MM_Iface_Push(MM_SQ, p_cmd, cmd_size)

/*! \fn int8_t SP_Iface_Pop_Cmd_From_MM2SP_CQ(void* rx_buff)
    \brief Pop response from to Master Minion (MM) to Service Processor (SP)
    Completion Queue(CQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
#define SP_Iface_Pop_Cmd_From_MM2SP_CQ(rx_buff)   \
    SP_MM_Iface_Pop(MM_CQ, rx_buff)

/*! \fn int8_t SP_Iface_Pop_Cmd_From_SP2MM_SQ(void* rx_buff)
    \brief Pop response from to Service Processor (SP) to Master Minion (MM)
    Submission Queue(SQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
#define SP_Iface_Pop_Cmd_From_SP2MM_SQ(rx_buff)   \
    SP_MM_Iface_Pop(SP_SQ, rx_buff)

/*! \fn int8_t SP_Iface_Push_Cmd_To_SP2MM_CQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Service Processor (SP) to Master Minion (MM)
    Completion Queue(CQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define SP_Iface_Push_Cmd_To_SP2MM_CQ(p_cmd, cmd_size)   \
    SP_MM_Iface_Push(SP_CQ, p_cmd, cmd_size)

/*! \fn void SP_Iface_Processing(void)
    \brief An API to process messages from SP on receving;
    1. MM2SP_CQ post notification
    2. SP2MM SQ post notification
    \return Status indicating success or negative error
*/
int8_t SP_Iface_Processing(void);

#endif
