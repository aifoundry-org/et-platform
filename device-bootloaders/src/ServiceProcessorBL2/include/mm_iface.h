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
*       SP to Minion communications
*
***********************************************************************/
#ifndef MM_IFACE_H
#define MM_IFACE_H

#include "etsoc/common/common_defs.h"
#include "transports/sp_mm_iface/sp_mm_iface.h" 
#include "FreeRTOS.h"

/*! \fn int8_t MM_Iface_Init(void)
    \brief This function initialise SP to Master Minion interface.
    \param none
    \return Status indicating success or negative error
*/
int8_t MM_Iface_Init(void);

/*! \fn int8_t MM_Iface_Send_Echo_Cmd(TickType_t timeout)
    \brief This sends Echo command to Master Minion. It is a blocking call
    and it waits for response for a given time.
    This is an example to SP2MM command.
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Send_Echo_Cmd(void);

/*! \fn int32_t MM_Iface_MM_Command_Shell(void* cmd, uint32_t cmd_size,
    char* rsp, uint32_t *rsp_size)
    \brief This interface receives TF command shell command, sends
    encapsulatedMM command to MM FW, wait and receive response from
    MM FW, wrap MM response in TF response shell, and return response.
    \return Status indicating success or negative error
*/
int32_t MM_Iface_MM_Command_Shell(void* cmd, uint32_t cmd_size,
    char* rsp, uint32_t *rsp_size);

/*! \fn int32_t MM_Iface_Get_DRAM_BW(uint32_t *read_bw, uint32_t *write_bw)
    \brief This sends Get DRAM BW command to Master Minion. It is a blocking call
    and it waits for response for a given time.
    \param read_bw response containing read BW
    \param write_bw response containing write BW
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Get_DRAM_BW(uint32_t *read_bw, uint32_t *write_bw);

/*! \fn int8_t MM_Iface_Push_Cmd_To_SP2MM_SQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Service Processor (SP) to Master Minion (MM)
    Submission Queue(SQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define MM_Iface_Push_Cmd_To_SP2MM_SQ(p_cmd, cmd_size)   \
        SP_MM_Iface_Push(MM_SQ, p_cmd, cmd_size, UNCACHED)

/*! \fn int8_t MM_Iface_Pop_Rsp_From_SP2MM_CQ(void* rx_buff)
    \brief Pop response from to Service Processor (SP) to Master Minion (MM)
    Completion Queue(CQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
#define MM_Iface_Pop_Rsp_From_SP2MM_CQ(rx_buff)          \
        SP_MM_Iface_Pop(MM_CQ, rx_buff, UNCACHED)

/*! \fn int32_t MM_Iface_Pop_Cmd_From_MM2SP_SQ(void* rx_buff)
    \brief Pop command from Master Minion (MM) to Service Processor (SP)
    Submission Queue(SQ)
    \param rx_buff Buffer to receive response popped
    \return Status indicating success or negative error
*/
int32_t MM_Iface_Pop_Cmd_From_MM2SP_SQ(void* rx_buff);

/*! \fn int8_t MM_Iface_Push_Rsp_To_MM2SP_CQ(void* p_cmd, uint32_t cmd_size)
    \brief Push response to Master Minion (MM) to Service Processor (SP)
    Completion Queue(CQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define MM_Iface_Push_Rsp_To_MM2SP_CQ(p_cmd, cmd_size)    \
        SP_MM_Iface_Push(SP_CQ, p_cmd, cmd_size, UNCACHED)

#endif
